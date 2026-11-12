extern "C" int printf(const char *, ...);

struct Global {
    int v;
    Global(int x) : v(x) { printf("Global ctor, v=%d\n", v); }
    ~Global() { printf("Global dtor, v=%d\n", v); }
};
static Global g1(41);

struct Shape { virtual ~Shape() {} virtual int area() const = 0; };
struct Sq : Shape { int s; Sq(int s) : s(s) {} int area() const override { return s * s; } };

int main(int argc, char **argv) {
    printf("main: argc=%d, g1.v=%d\n", argc, g1.v);
    Shape *p = new Sq(argc + 2);
    printf("virtual call: area=%d\n", p->area());
    delete p;
    int *a = new int[4];
    for (int i = 0; i < 4; i++) a[i] = i * i;
    printf("new[]: %d %d %d %d\n", a[0], a[1], a[2], a[3]);
    delete[] a;
    return 0;
}
