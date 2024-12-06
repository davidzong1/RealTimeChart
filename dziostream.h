#ifndef __DZIOSTREAM_H__
#define __DZIOSTREAM_H__
#include <QMainWindow>
#include <QTextEdit>
#include <iostream>
#include <streambuf>

class QTextEditStream : public std::ostream
{
public:
    QTextEditStream(QTextEdit *textEdit)
        : std::ostream(&buffer), buffer(textEdit) {}

private:
    class Buffer : public std::streambuf
    {
    public:
        Buffer(QTextEdit *textEdit) : textEdit(textEdit)
        {
            setp(buffer, buffer + sizeof(buffer) - 1); // 设置初始缓冲区指针
        }

        int sync() override
        {
            if (this->pbase() != this->pptr())
            { // 检查是否有数据需要刷新
                QString message = QString::fromUtf8(this->pbase(), this->pptr() - this->pbase());
                textEdit->moveCursor(QTextCursor::End); // 移动光标到文档末尾
                // 设置文本颜色为黑色
                QTextCharFormat format;
                format.setForeground(Qt::black);
                textEdit->mergeCurrentCharFormat(format);
                textEdit->insertPlainText(message); // 插入新文本
                textEdit->ensureCursorVisible();    // 确保光标可见
                // 重置缓冲区指针
                this->setp(buffer, buffer + sizeof(buffer) - 1);
            }
            return 0;
        }

        int_type overflow(int_type v) override
        {
            if (v == traits_type::eof())
            {
                return traits_type::not_eof(v);
            }
            else
            {
                *pptr() = static_cast<char>(v);
                pbump(1);
                return sync() == 0 ? v : traits_type::eof();
            }
        }

    private:
        QTextEdit *textEdit;
        char buffer[128]; // 缓冲区大小可以根据需求调整
    } buffer;
};
#endif
