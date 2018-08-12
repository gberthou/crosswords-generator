#ifndef UI_HPP
#define UI_HPP

#include <vector>
#include <string>
#include <regex>

#include <ncurses.h>

#include "crosswords.hpp"

enum UI_State
{
    UI_IDLE,
    UI_EXIT,
    UI_EDITH,
    UI_EDITV
};

enum UI_Color
{
    UI_COL_SELECTED = 1,
    UI_COL_EDITH,
    UI_COL_EDITV
};

class UI
{
    public:
        UI(size_t width, size_t height):
            width(width),
            height(height),
            cursorX(0),
            cursorY(0),
            state(UI_IDLE),
            letters(width*height, '.')
        {
            initscr();
            noecho();
            keypad(stdscr, TRUE);

            start_color();
            init_pair(UI_COL_SELECTED, COLOR_BLACK, COLOR_WHITE);
            init_pair(UI_COL_EDITH, COLOR_GREEN, COLOR_WHITE);
            init_pair(UI_COL_EDITV, COLOR_BLUE, COLOR_WHITE);
        }

        ~UI()
        {
            endwin();
        }


        void loop()
        {
            while(state != UI_EXIT)
            {
                display();

                int c = getch();
                process_input(c);
            }
        }

    protected:
        int &current_letter()
        {
            return letters[cursorX + cursorY * width];
        }

        static void extract_words_line(std::vector<std::string> &words, std::string line)
        {
            std::regex r1("^([a-z]+)$|^([a-z]{2,})#|#([a-z]{2,})#|#([a-z]{2,})$");
            std::regex r2("[a-z]+");
            std::smatch match;

            while(std::regex_search(line, match, r1))
            {
                std::smatch m;
                std::string tmp = match.str();
                std::regex_search(tmp, m, r2);

                words.push_back(m.str());
                line = match.suffix();
            }
        }

        void extract_words(std::vector<std::string> &words) const
        {
            words.clear();
            for(size_t y = 0; y < height; ++y)
            {
                std::string s("");
                for(size_t x = 0; x < width; ++x)
                    s += letters[x + y * width];
                extract_words_line(words, s);
            }

            for(size_t x = 0; x < width; ++x)
            {
                std::string s("");
                for(size_t y = 0; y < height; ++y)
                    s += letters[x + y * width];
                extract_words_line(words, s);
            }
        }

        void color(bool enable) const
        {
            UI_Color color;
            switch(state)
            {
                case UI_EDITH:
                    color = UI_COL_EDITH;
                    break;

                case UI_EDITV:
                    color = UI_COL_EDITV;
                    break;

                default:
                    color = UI_COL_SELECTED;
                    break;
            }

            if(enable)
                attron(COLOR_PAIR(color));
            else
                attroff(COLOR_PAIR(color));
        }

        void display() const
        {
            std::vector<int>::const_iterator it = letters.begin();

            for(size_t y = 0; y < height; ++y)
            {
                for(size_t x = 0; x < width; ++x)
                {
                    bool selected = (x == cursorX && y == cursorY)
                                 || (state == UI_EDITH && x >= selectFrom && x <= cursorX && y == cursorY)
                                 || (state == UI_EDITV && y >= selectFrom && y <= cursorY && x == cursorX);

                    if(selected)
                        color(true);
                    mvaddch(y, x, *it);
                    mvaddch(x, y + width + 4, *it++);
                    if(selected)
                        color(false);
                }
            }
            refresh();
        }

        bool try_replace_letter(int c, bool horizontal)
        {
            if(c >= 'A' && c <= 'Z')
                c += 32;
            if(c >= 'a' && c <= 'z')
            {
                current_letter() = c;
                if(horizontal)
                {
                    ++cursorX;
                    if(cursorX >= width)
                    {
                        cursorX = width-1;
                        return true;
                    }
                }
                else
                {
                    ++cursorY;
                    if(cursorY >= height)
                    {
                        cursorY = height-1;
                        return true;
                    }
                }
            }
            return false;
        }

        void process_input(int c)
        {
            switch(state)
            {
                case UI_EDITH:
                    if(c == '\n')
                        state = UI_IDLE;
                    if(try_replace_letter(c, true))
                        state = UI_IDLE;
                    break;

                case UI_EDITV:
                    if(c == '\n')
                        state = UI_IDLE;
                    if(try_replace_letter(c, false))
                        state = UI_IDLE;
                    break;

                default: // UI_IDLE
                    switch(c)
                    {
                        case 'q':
                        case 'Q':
                            state = UI_EXIT;
                            break;

                        case 'i':
                            state = UI_EDITH;
                            selectFrom = cursorX;
                            break;

                        case 'I':
                            state = UI_EDITV;
                            selectFrom = cursorY;
                            break;

                        case 'x':
                        case 'X':
                            current_letter() = '.';
                            break;

                        case ' ':
                            current_letter() = '#';
                            break;

                        case 's':
                        case 'S':
                        {
                                std::vector<std::string> words;
                                extract_words(words);
                                if(Crosswords::solve(width, height, letters, words))
                                {
                                    std::cout << "Found!" << std::endl;
                                }
                                else
                                {
                                    std::cout << "Not found..." << std::endl;
                                }
                            break;
                        }

                        case KEY_LEFT:
                            if(cursorX)
                                --cursorX;
                            break;

                        case KEY_RIGHT:
                            if(cursorX+1 < width)
                                ++cursorX;
                            break;

                        case KEY_UP:
                            if(cursorY)
                                --cursorY;
                            break;

                        case KEY_DOWN:
                            if(cursorY+1 < height)
                                ++cursorY;
                            break;

                        default:
                            break;
                    }
                    break;
            }
        }

        size_t width;
        size_t height;
        size_t cursorX;
        size_t cursorY;
        size_t selectFrom;
        UI_State state;

        std::vector<int> letters;
};

#endif
