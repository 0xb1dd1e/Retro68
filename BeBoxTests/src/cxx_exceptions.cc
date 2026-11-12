#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

struct Tracker {
    const char *name;
    Tracker(const char *n) : name(n) { std::printf("  ctor %s\n", name); }
    ~Tracker() { std::printf("  dtor %s (unwound)\n", name); }
};

static void thrower(int what) {
    Tracker t("in thrower");
    if (what == 1) throw 42;
    if (what == 2) throw std::runtime_error("runtime_error from thrower");
    if (what == 3) throw std::string("a std::string");
}

int main(int argc, char **argv) {
    std::printf("== exceptions\n");
    for (int w = 1; w <= 3; w++) {
        try {
            thrower(w);
            std::printf("no throw?\n");
        } catch (int i) {
            std::printf("caught int %d\n", i);
        } catch (const std::exception &e) {
            std::printf("caught std::exception: %s\n", e.what());
        } catch (const std::string &s) {
            std::printf("caught string: %s\n", s.c_str());
        }
    }
    std::printf("== std::string / std::vector\n");
    std::string s = "Hello";
    s += ", BeBox ";
    s += std::to_string(argc);
    std::printf("%s (len %u)\n", s.c_str(), (unsigned)s.size());
    std::vector<int> v;
    for (int i = 0; i < 10; i++) v.push_back(i * i);
    int sum = 0;
    for (int x : v) sum += x;
    std::printf("vector size %u, sum %d\n", (unsigned)v.size(), sum);
    std::printf("== done\n");
    return 0;
}
