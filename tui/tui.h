#include <iostream>
#include <string>
#include <sstream>
#include <atomic>
#include <sys/signal.h>
#include <termios.h>
#include <unistd.h>

namespace TUI {

// Represents a keydown event
class KeyDown {
    bool alt_on;
    char symbol;
};


// 24-bit color
struct RGB{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    bool operator ==(RGB const& other);
    bool operator !=(RGB const& other);
};

// Represents a unicode symbol, foreground color, and
// background color, for use in Canvases
struct Tile {
    // This is a string, rather than a character,
    // to support unicode rendering
    std::string symbol;

    // The foreground and background color
    RGB fore_color;
    RGB back_color;

    Tile();
    Tile(std::string symbol, RGB fore, RGB back);
    Tile(RGB color);

    operator std::string();
    std::string raw_symbol();
};


class Canvas {

    // Dimensions
    size_t width;
    size_t height;

    // Offset
    size_t offset_x;
    size_t offset_y;

    // A tile array representing the last image displayed
    Tile *prev_buffer;

    // A tile array representing the image that would be
    // output the next time a display occurs
    Tile *tile_buffer;


    public:
    Canvas(size_t width, size_t height, size_t x, size_t y);
    Canvas(size_t width, size_t height);

    void resize(size_t width, size_t height);
    void reposition(size_t x, size_t y);
    Tile& operator()(size_t x, size_t y);

    void hide();
    void full_display();
    void lazy_display();

    size_t get_width();
    size_t get_height();
};


class TextBox : protected Canvas {

    std::stringstream content;

    public:

    TextBox(size_t width, size_t height, size_t x, size_t y);
    TextBox(size_t width, size_t height);

    void resize(size_t width, size_t height);
    void reposition(size_t x, size_t y);

    void hide();
    void full_display();
    void lazy_display();

    size_t get_width();
    size_t get_height();

    template<typename T>
    TextBox &operator<<(T value) {
        content << value;
        return *this;
    }

};


// Used to configure the way the program recieves input
class Input {

    // Stores the user's default settings, for later resoration
    static termios original_termios;

    public:

    static void cooked_mode();
    static void last_meal(int signal);
    static void raw_mode();
};




};
