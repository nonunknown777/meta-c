package TYPES_DEMO

using IO

block global = 64MB
block temp = 8MB

interface Drawable {
    fn draw()
}

interface Area {
    fn area() -> f64
}

struct Circle {
    u32 id
    String name
    f32 radius

    fn Circle(u32 i, String n, f32 r) {
        id = i
        name = n
        radius = r
    }

    fn draw() {
        print("Circle #{0} '{1}' radius={2}", id, name, radius)
    }

    fn area() -> f64 {
        f64 r2 = radius
        return 3.141592653589793f64 * r2 * r2
    }
}

struct Rectangle {
    u32 id
    String name
    f64 width
    f64 height

    fn Rectangle(u32 i, String n, f64 w, f64 h) {
        id = i
        name = n
        width = w
        height = h
    }

    fn draw() {
        print("Rect #{0} '{1}' {2}x{3}", id, name, width, height)
    }

    fn area() -> f64 {
        return width * height
    }
}

fn main() {
    print("=== Fixed-Width Types & Interfaces ===")

    Circle c = Circle(1u32, "Small Circle", 5.0f32) @temp
    Rectangle r = Rectangle(2u32, "Big Rect", 10.0, 20.0) @global

    c.draw()
    r.draw()

    f64 total = c.area() + r.area()
    print("Total area = {0}", total)

    i32 sum = 0
    i32 i = 0
    while i < 10 {
        sum = sum + i
        i = i + 1
    }
    print("Sum 0..9 = {0}", sum)

    i32 neg = -128i32
    u8 small = 255u8

    if small == 255u8 {
        print("u8 max is {0}", small)
    }

    print("neg={0}", neg)
    print("Done!")
    temp.reset()
    global.reset()
}
