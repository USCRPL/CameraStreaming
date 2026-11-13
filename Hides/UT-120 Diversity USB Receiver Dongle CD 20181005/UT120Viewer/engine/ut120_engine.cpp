// ut120_engine - drives the UT-120 (ITE IT913x) directly over WinUSB and streams
// the raw transport stream to stdout. No vendor kernel driver is involved.
//
//   stdout : raw MPEG-TS bytes
//   stderr : one JSON event per line
//   stdin  : commands, one per line:
//              open <device-index>
//              tune <freq-khz> <bandwidth-mhz>
//              close
//              quit
//   ut120_engine list      prints the dongles as JSON and exits
//   ut120_engine selftest  opens the first dongle, loads firmware, prints chip info
//
// Closing stdin ends the process, so the engine cannot outlive the GUI.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ut120_hw.h"

// ---------------------------------------------------------------- events

static std::mutex g_eventLock;

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += (char)c; }
        else if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); out += b; }
        else out += (char)c;
    }
    return out;
}

static void emitLine(const std::string& json) {
    std::lock_guard<std::mutex> lock(g_eventLock);
    fprintf(stderr, "%s\n", json.c_str());
    fflush(stderr);
}

static void emitError(const std::string& message) {
    emitLine("{\"event\":\"error\",\"message\":\"" + jsonEscape(message) + "\"}");
}

static std::string hex(unsigned long v) {
    char b[16];
    snprintf(b, sizeof b, "0x%08lX", v);
    return b;
}

// ---------------------------------------------------------------- TS output

// USB reads must never wait on the GUI, so buffers go through a bounded queue
// to a writer thread; if the reader stalls, data is dropped and counted.
class TsWriter {
public:
    void start() {
        stop_ = false;
        thread_ = std::thread([this] { run(); });
    }
    void stop() {
        { std::lock_guard<std::mutex> l(m_); stop_ = true; }
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
    }
    void push(const unsigned char* data, size_t n) {
        total += n;
        {
            std::lock_guard<std::mutex> l(m_);
            if (queued_ + n > kMax) { dropped += n; return; }
            q_.emplace_back(data, data + n);
            queued_ += n;
        }
        cv_.notify_one();
    }
    std::atomic<unsigned long long> total{0}, dropped{0};
private:
    static constexpr size_t kMax = 32 * 1024 * 1024;
    void run() {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        for (;;) {
            std::vector<unsigned char> chunk;
            {
                std::unique_lock<std::mutex> l(m_);
                cv_.wait(l, [this] { return stop_ || !q_.empty(); });
                if (q_.empty()) return;
                chunk = std::move(q_.front());
                q_.pop_front();
                queued_ -= chunk.size();
            }
            const unsigned char* p = chunk.data();
            size_t left = chunk.size();
            while (left) {
                DWORD wrote = 0;
                if (!WriteFile(out, p, (DWORD)left, &wrote, nullptr) || !wrote) return;
                p += wrote;
                left -= wrote;
            }
        }
    }
    std::mutex m_;
    std::condition_variable cv_;
    std::deque<std::vector<unsigned char>> q_;
    size_t queued_ = 0;
    bool stop_ = false;
    std::thread thread_;
};

static TsWriter g_writer;

static void onTs(const unsigned char* data, unsigned long len, void*) { g_writer.push(data, len); }

// ---------------------------------------------------------------- device

class Device {
public:
    bool open(int index) {
        close();
        char err[256] = {0};
        if (hw_open(index, err, sizeof err) != 0) {
            emitError(err);
            return false;
        }
        g_writer.start();
        reader_ = std::thread([] {
            int rc = hw_stream(onTs, nullptr);
            if (rc) emitError("TS read stopped (error " + std::to_string(rc) + ")");
        });
        open_ = true;
        hw_device devs[8];
        int n = hw_list(devs, 8);
        std::string name = index < n ? devs[index].name : "UT-120";
        emitLine("{\"event\":\"opened\",\"index\":" + std::to_string(index) + ",\"name\":\"" + jsonEscape(name) +
                 "\",\"info\":" + hw_info() + "}");
        return true;
    }
    void close() {
        if (!open_.exchange(false)) return;
        hw_stream_stop();
        if (reader_.joinable()) reader_.join();
        g_writer.stop();
        hw_close();
    }
    void tune(long khz, long bw) {
        if (!open_) { emitError("Tuner is not open"); return; }
        unsigned long e = hw_tune((unsigned long)khz, (unsigned)bw);
        if (e) { emitError("Tune failed (" + hex(e) + ")"); return; }
        emitLine("{\"event\":\"tuned\",\"khz\":" + std::to_string(khz) + ",\"bw\":" + std::to_string(bw) + "}");
    }
    void poll() {
        if (!open_) return;
        hw_stats s;
        unsigned long e = hw_get_stats(&s);
        if (e) { emitError("Statistics read failed (" + hex(e) + ")"); return; }
        int locked = 0, present = 0, strength = 0, quality = 0;
        std::string chips = "[";
        for (int c = 0; c < s.chips; ++c) {
            locked |= s.locked[c];
            present |= s.present[c];
            if (s.strength[c] > strength) strength = s.strength[c];
            if (s.quality[c] > quality) quality = s.quality[c];
            char b[256];
            snprintf(b, sizeof b,
                     "%s{\"locked\":%s,\"strength\":%d,\"quality\":%d,\"dbm\":%ld,\"snr\":%d,\"ber\":%.3e,\"aborts\":%u}",
                     c ? "," : "", s.locked[c] ? "true" : "false", s.strength[c], s.quality[c], s.dbm[c],
                     s.snr_db[c], s.ber[c], s.aborts[c]);
            chips += b;
        }
        chips += "]";
        emitLine("{\"event\":\"signal\",\"locked\":" + std::string(locked ? "true" : "false") +
                 ",\"present\":" + (present ? "true" : "false") +
                 ",\"strength\":" + std::to_string(strength) +
                 ",\"quality\":" + std::to_string(quality) +
                 ",\"chips\":" + chips +
                 ",\"bytes\":" + std::to_string(g_writer.total.load()) +
                 ",\"dropped\":" + std::to_string(g_writer.dropped.load()) + "}");
    }
private:
    std::atomic<bool> open_{false};
    std::thread reader_;
};

// ---------------------------------------------------------------- entry

static int cmdList() {
    hw_device devs[8];
    int n = hw_list(devs, 8);
    std::string out = "[";
    for (int i = 0; i < n; ++i) {
        if (i) out += ",";
        out += "{\"index\":" + std::to_string(i) + ",\"name\":\"" + jsonEscape(devs[i].name) +
               "\",\"driver\":\"" + jsonEscape(devs[i].service) + "\",\"ready\":" + (devs[i].ready ? "true" : "false") + "}";
    }
    printf("%s]\n", out.c_str());
    return 0;
}

static int cmdSelfTest() {
    hw_device devs[8];
    int n = hw_list(devs, 8);
    printf("dongles found: %d\n", n);
    for (int i = 0; i < n; ++i)
        printf("  [%d] %s  driver=%s  %s\n", i, devs[i].name, devs[i].service[0] ? devs[i].service : "(none)",
               devs[i].ready ? "ready" : "NEEDS WinUSB");
    if (!n) return 1;
    char err[256] = {0};
    DWORD t = GetTickCount();
    if (hw_open(0, err, sizeof err) != 0) { printf("open failed: %s\n", err); return 1; }
    printf("opened in %lu ms: %s\n", GetTickCount() - t, hw_info());
    hw_stats s;
    unsigned long e = hw_get_stats(&s);
    printf("statistics read: %s\n", e ? hex(e).c_str() : "ok");
    hw_close();
    return 0;
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "run";
    if (mode == "list") return cmdList();
    if (mode == "selftest") return cmdSelfTest();

    Device device;
    std::mutex cmdLock;   // stats polling and commands both drive the demodulator
    std::atomic<bool> quit{false};
    std::thread poller([&] {
        while (!quit) {
            Sleep(500);
            std::lock_guard<std::mutex> l(cmdLock);
            device.poll();
        }
    });

    char line[256];
    emitLine("{\"event\":\"ready\"}");
    while (fgets(line, sizeof line, stdin)) {
        char cmd[32] = {0};
        long a = 0, b = 0;
        int n = sscanf(line, "%31s %ld %ld", cmd, &a, &b);
        if (n < 1) continue;
        std::string c = cmd;
        if (c == "quit") break;
        std::lock_guard<std::mutex> l(cmdLock);
        if (c == "open" && n >= 2) device.open((int)a);
        else if (c == "close") device.close();
        else if (c == "tune" && n >= 3) device.tune(a, b);
        else emitError("bad command");
    }
    quit = true;
    poller.join();
    device.close();
    return 0;
}
