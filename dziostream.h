#ifndef __DZIOSTREAM_H__
#define __DZIOSTREAM_H__
#include <QMainWindow>
#include <QTextEdit>
#include <iostream>
#include <streambuf>

class QTextEditStream : public std::ostream
{
public:
    QTextEditStream(QTextEdit *textEdit) : std::ostream(&buffer), buffer(textEdit)
    {
    }

private:
    class Buffer : public std::streambuf
    {
    public:
        Buffer(QTextEdit *textEdit) : textEdit(textEdit)
        {
            setp(buffer, buffer + sizeof(buffer) - 1);
        }

        int sync() override
        {
            if (this->pbase() != this->pptr())
            {
                // 使用QByteArray确保正确处理UTF-8
                QByteArray utf8Data(this->pbase(), this->pptr() - this->pbase());
                QString message = QString::fromUtf8(utf8Data);

                textEdit->moveCursor(QTextCursor::End);
                QTextCharFormat format;
                format.setForeground(Qt::black);
                textEdit->mergeCurrentCharFormat(format);
                textEdit->insertPlainText(message);
                textEdit->ensureCursorVisible();

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

            // 处理UTF-8字符
            *pptr() = static_cast<char>(v);
            pbump(1);

            return sync() == 0 ? v : traits_type::eof();
        }

    private:
        QTextEdit *textEdit;
        char buffer[1024]; // 增大缓冲区以容纳更多UTF-8字符
    } buffer;
};
#endif
