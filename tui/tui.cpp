#include "tui.h"

using namespace TUI;


bool RGB::operator==(RGB const& other) {
    return    (other.red   == red  )
           && (other.green == green)
           && (other.blue  == blue );
}

bool RGB::operator!=(RGB const& other) {
    return  ! (*this == other);
}


Tile::Tile() : symbol("\0"), fore_color({0,0,0}), back_color({0,0,0}) {}

Tile::Tile(std::string symbol, RGB fore, RGB back)
    : symbol(symbol)
    , fore_color(fore)
    , back_color(back)
{}

Tile::Tile(RGB color)
    : symbol(" ")
    , fore_color({0,0,0})
    , back_color(color)
{}


// Uses ANSI escape sequences to change the foreground color
// and background color of the contained symbol to reflect the
// fore_color and back_color of the tile.
Tile::operator std::string() {
    std::stringstream ss;
    if(symbol != " "){
        ss << "\033[38;2;"
           << (int) fore_color.red   << ";"
           << (int) fore_color.green << ";"
           << (int) fore_color.blue  << "m";
    }
    ss << "\033[48;2;"
       << (int) back_color.red   << ";"
       << (int) back_color.green << ";"
       << (int) back_color.blue  << "m";
    ss << symbol;
    return ss.str();
}

// Returns the symbol string without any ANSI escaping
std::string Tile::raw_symbol() {
    return symbol;
}


Canvas::Canvas(size_t width, size_t height, size_t x, size_t y)
    : width(width)
    , height(height)
    , offset_x(x)
    , offset_y(y)
    , prev_buffer(new Tile[height*width])
    , tile_buffer(new Tile[height*width])
{}


Canvas::Canvas(size_t width, size_t height)
    : width(width)
    , height(height)
    , offset_x(0)
    , offset_y(0)
    , prev_buffer(new Tile[height*width])
    , tile_buffer(new Tile[height*width])
{}

void Canvas::resize(size_t width, size_t height) {
    Tile *new_tile_buffer = new Tile[height*width];
    Tile *new_prev_buffer = new Tile[height*width];
    size_t x_limit = std::min(this->width,width);
    size_t y_limit = std::min(this->width,width);
    for (size_t y=0; y<y_limit; y++) {
        for (size_t x=0; x<x_limit; x++) {
            new_tile_buffer[y*width+x] = (*this)(x,y);
            new_prev_buffer[y*width+x] = (*this)(x,y);
        }
    }
    delete[] tile_buffer;
    delete[] prev_buffer;
    tile_buffer = new_tile_buffer;
    prev_buffer = new_prev_buffer;
}

void Canvas::reposition(size_t x, size_t y) {
    hide();
    offset_x = x;
    offset_y = y;
    full_display();
}


Tile& Canvas::operator()(size_t x, size_t y) {
    if( (x<0) || (x>=width) || (y<0) || (y>=height) ){
        std::stringstream ss;
        ss << "Canvas with dimensions ("
           << width << ',' << height
           << ") accessed out of bounds with coordinates ("
           << x << ',' << y << ')';
        throw std::runtime_error(ss.str());
    }
    return tile_buffer[y*width+x];
}

void Canvas::hide() {
    std::string output;
    output += "\033[s";
    // Handle y offset
    if (offset_y != 0){
        output += "\033[" + std::to_string(offset_y) + "B";
    }
    // Switch to default colors
    output += "\033[39m\033[49m";
    for (size_t y=0; y<height; y++) {
        // Handle x offset
        output += "\033[" + std::to_string(offset_x+1) + "G";
        for (size_t x=0; x<width; x++) {
            // Just write spaces everywhere
            output += ' ';
        }
        output += "\r\n";
    }
    output += "\033[u";
    std::cout << output;
}


// Draw the entire canvas to the terminal
void Canvas::full_display() {
    std::string output;
    Tile *last_tile = nullptr;
    output += "\033[s";
    // Handle y offset
    if (offset_y != 0) {
        output += "\033[" + std::to_string(offset_y) + "B";
    }
    bool mismatch;
    for (int y=0; y<height; y++) {
        mismatch = true;
        for (int x=width-1; x>=0; x--) {
            size_t index = y*width+x;
            // Handle x offset
            output += "\033[" + std::to_string(offset_x+x+1) + "G";

            Tile &current_tile = tile_buffer[index];
            if (last_tile != nullptr) {
                mismatch |= last_tile->fore_color != current_tile.fore_color;
                mismatch |= last_tile->back_color != current_tile.back_color;
            }
            last_tile = &current_tile;
            // Write out tile, avoiding escapes if they aren't necessary
            if (mismatch) {
                output += (std::string) current_tile;
            } else {
                output += current_tile.raw_symbol();
            }
            mismatch = false;
            // Record the state of the written tile
            prev_buffer[index] = current_tile;
        }
        // Escape to default colors when  moving to the next line
        output += "\033[39m\033[49m";
        output += "\r\n";
    }
    output += "\033[u";
    std::cout << output;
}


// This function displays much faster, because it only updates tiles that
// have changed, but it requires `full_display` to be called once after
// the canvas is constructed or resized.
void Canvas::lazy_display() {
    std::string output;
    Tile *last_tile = nullptr;

    // Save cursor position
    output += "\033[s";

    // Handle y offset
    if (offset_y != 0) {
        output += "\033[" + std::to_string(offset_y) + "B";
    }

    // Save position of the last tile we had to update
    int last_y = 0;
    int last_x = -1;

    bool first = true;
    for (int y=0; y<height; y++) {
        // Indicates the first tile in a row
        for (int x=width-1; x>=0; x--) {

            // Get references to the previously displayed tile state for
            // this positon and the state that must now be displayed
            int index = y*width+x;
            Tile &prev_state = prev_buffer[y*width+x];
            Tile &next_state = tile_buffer[y*width+x];

            // Current tile needs to be updated if:
            //     - foreground color changes
            //     - background color changes
            //     - symbol changes
            bool touched = (prev_state.fore_color != next_state.fore_color);
            touched = touched || (prev_state.back_color != next_state.back_color);
            touched = touched || (prev_state.symbol != next_state.symbol);
            //touched = true;

            // Only re-display a tile if it an update is required
            if (touched) {

                // Adjust cursor x and y coordinates from last displayed tile
                // position to the current position

                // We move the cursor horizontally from right to left and by
                // absolute positon to account for multi-column symbols (eg: emoji)
                output += "\033[" + std::to_string(offset_x+x+1) + "G";

                // We move the cursor from the top down and by relative position so that
                // we can lock the canvas to a specific scroll position, meaning we don't
                // destroy any of the terminal's previously printed lines.
                if ( y != last_y ) {
                    int delta = y - last_y;
                    char trailer = (delta > 0) ? 'B' : 'A';
                    output += "\033[" + std::to_string(std::abs(delta)) + trailer;
                }
                last_x = x;
                last_y = y;

                // Whether or not the tile we are scanning through has colors
                // matching the previous tile we actually updated
                bool mismatch = first;
                if (last_tile != nullptr) {
                    mismatch |= last_tile->fore_color != next_state.fore_color;
                    mismatch |= last_tile->back_color != next_state.back_color;
                }
                first = false;

                // Update our prev_buffer to reflect the symbol that will be displayed
                prev_state = next_state;

                // If the colors don't match, add in the appropriate color escapes,
                // otherwise just print the symbol
                if(mismatch){
                    output += (std::string) tile_buffer[y*width+x];
                } else {
                    output += tile_buffer[y*width+x].raw_symbol();
                }

                // Remember the tile we most recently displayed
                last_tile = &next_state;
            }
        }
    }

    // restore the previously saved cursor position
    output += "\033[u";
    // Set the foreground and background colors back to their defaults, just in case
    output += "\033[39m\033[49m";
    std::cout << output;
    std::cout.flush();
}

size_t Canvas::get_width() {
    return width;
}

size_t Canvas::get_height() {
    return height;
}



// Adapted from the interesting tutorial at:
// https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
// which is published under CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
void Input::cooked_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &Input::original_termios);
}

void Input::last_meal(int signal) {
    // Escape out of raw mode
    Input::cooked_mode();
    // Reset colors to their defaults
    std::cout << "\033[39m\033[49m";
    // Make sure the reset occurs before exit
    std::cout.flush();
    // Kill the program
    exit(1);
}

void Input::raw_mode() {
    tcgetattr(STDIN_FILENO, &Input::original_termios);
    atexit(cooked_mode);
    signal(SIGINT, Input::last_meal);
    signal(SIGSEGV,Input::last_meal);
    termios raw_termios = Input::original_termios;
    raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw_termios.c_oflag &= ~(OPOST);
    raw_termios.c_cflag |= (CS8);

    //// This line would remove the conversion of ctrl-C and ctrl-Z
    //// into signals. For safety, this is excluded.
    // raw_termios.c_lflag &= ~(ISIG);
    raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios);
}

termios Input::original_termios;
