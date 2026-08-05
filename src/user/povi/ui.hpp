#ifndef POVI_UI_HPP
#define POVI_UI_HPP

#include "editor.hpp"

/* 屏幕渲染：清屏后重绘内容区（24 行）与状态栏（最后一行）。 */
class UI
{
public:
        void refresh(Editor &ed);

private:
        void write_line(const char *s);
        void write_status(const Editor &ed);
};

#endif
