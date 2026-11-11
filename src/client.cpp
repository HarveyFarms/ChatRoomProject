
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFontDatabase>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTimer>

class TerminalWindow : public QWidget
{
    QTcpSocket socket;
    QTimer timeout;

    QPlainTextEdit* output;
    QLineEdit* input;
    QLabel* status;

    QString username;
    QString currentChannel;

    bool joined = false;

    void print(const QString& s)
    {
        output->appendPlainText(s);
    }

    void send(const QJsonObject& m)
    {
        if (socket.bytesToWrite() > 65536)
        {
            print("[error] Connection is backed up; try again shortly.");
            return;
        }

        socket.write(
            QJsonDocument(m).toJson(QJsonDocument::Compact) + '\n'
        );
    }

    void updateStatus()
    {
        if (!joined)
        {
            status->setText("OFFLINE");
        }
        else if (currentChannel.isEmpty())
        {
            status->setText("ONLINE / NO CHANNEL / " + username);
        }
        else
        {
            status->setText(
                "ONLINE / #" + currentChannel + " / " + username
            );
        }
    }

    void help()
    {
        print(
            "CHAT ROOM / CHANNEL EDITION\n"
            "  /connect HOST PORT NAME  - connect to a server\n"
            "  /register EMAIL PASSWORD - create an account\n"
            "  /login PASSWORD          - log in to your account\n"
            "  /users                   - list online users\n"
            "  /channels                - list available channels\n"
            "  /create-channel NAME     - create a channel\n"
            "  /join NAME               - join a channel\n"
            "  /leave                   - leave current channel\n"
            "  /disconnect              - leave the server\n"
            "  /clear   /help   /quit\n\n"
            "Example:\n"
            "  /connect 127.0.0.1 5050 Ryan\n"
            "  /register ryan@example.com TestPass123\n"
            "  /login TestPass123\n"
            "  /channels\n"
            "  /create-channel gaming\n"
            "  /join gaming\n\n"
            "After joining a channel, type a message and press Enter.\n"
        );
    }

    void command()
    {
        const QString text = input->text().trimmed();
        input->clear();

        if (text.isEmpty())
        {
            return;
        }

        if (text == "/clear")
        {
            output->clear();
            return;
        }

        if (text == "/help")
        {
            help();
            return;
        }

        if (text == "/quit")
        {
            close();
            return;
        }

        if (text == "/disconnect")
        {
            timeout.stop();
            socket.abort();

            joined = false;
            currentChannel.clear();

            updateStatus();
            return;
        }

        const auto parts =
            text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts[0] == "/connect")
        {
            if (socket.state() != QAbstractSocket::UnconnectedState)
            {
                print("[info] Use /disconnect before connecting again.");
                return;
            }

            bool ok = false;

            int port = parts.size() == 4
                ? parts[2].toInt(&ok)
                : 0;

            if (!ok ||
                port < 1 ||
                port > 65535 ||
                !QRegularExpression("^[A-Za-z0-9_-]{1,24}$")
                    .match(parts.value(3)).hasMatch())
            {
                print(
                    "Usage: /connect HOST PORT NAME "
                    "(name: 1-24 letters, digits, _ or -)"
                );
                return;
            }

            username = parts[3];

            print("[system] Connecting to " +
                  parts[1] + ":" + parts[2] + " ...");

            status->setText("CONNECTING ...");
            timeout.start(10000);

            socket.connectToHost(
                parts[1],
                static_cast<quint16>(port)
            );

            return;
        }

        // Registration
        if (parts[0] == "/register")
        {
            if (socket.state() != QAbstractSocket::ConnectedState)
            {
                print("[error] Connect to a server first.");
                return;
            }

            if (joined)
            {
                print("[error] Disconnect before registering another account.");
                return;
            }

            if (parts.size() != 3)
            {
                print("Usage: /register EMAIL PASSWORD");
                return;
            }

            send({
                {"type", "register"},
                {"username", username},
                {"email", parts[1]},
                {"password", parts[2]}
            });

            print("[system] Registration request sent.");
            return;
        }

        // Login
        if (parts[0] == "/login")
        {
            if (socket.state() != QAbstractSocket::ConnectedState)
            {
                print("[error] Connect to a server first.");
                return;
            }

            if (joined)
            {
                print("[info] You are already logged in.");
                return;
            }

            if (parts.size() != 2)
            {
                print("Usage: /login PASSWORD");
                return;
            }

            send({
                {"type", "login"},
                {"username", username},
                {"password", parts[1]}
            });

            print("[system] Login request sent.");
            return;
        }

        // All commands below require authentication
        if (!joined)
        {
            print("[info] Log in first. Type /help.");
            return;
        }

        // List channels
        if (parts[0] == "/channels")
        {
            if (parts.size() != 1)
            {
                print("Usage: /channels");
                return;
            }

            send({
                {"type", "channels"}
            });

            return;
        }

        // Create a channel
        if (parts[0] == "/create-channel")
        {
            if (parts.size() != 2)
            {
                print("Usage: /create-channel NAME");
                return;
            }

            send({
                {"type", "create_channel"},
                {"name", parts[1]}
            });

            return;
        }

        // Join a channel
        if (parts[0] == "/join")
        {
            if (parts.size() != 2)
            {
                print("Usage: /join NAME");
                return;
            }

            send({
                {"type", "join_channel"},
                {"name", parts[1]}
            });

            return;
        }

        // Leave current channel
        if (parts[0] == "/leave")
        {
            if (parts.size() != 1)
            {
                print("Usage: /leave");
                return;
            }

            send({
                {"type", "leave_channel"}
            });

            return;
        }

        // List online users
        if (text == "/users")
        {
            send({
                {"type", "users"}
            });

            return;
        }

        // Unknown commands
        if (text.startsWith('/'))
        {
            print("Unknown command. Type /help.");
            return;
        }

        // Send a chat message
        if (currentChannel.isEmpty())
        {
            print("[info] Join a channel before sending messages.");
            return;
        }

        send({
            {"type", "chat"},
            {"text", text}
        });
    }

    
    void receive()
    {
        while (socket.canReadLine())
        {
            auto line = socket.readLine(8193);

            if (line.size() > 8192 || !line.endsWith('\n'))
            {
                print("[error] Invalid server frame.");
                socket.abort();
                return;
            }

            QJsonParseError parse;

            auto doc = QJsonDocument::fromJson(line, &parse);

            if (parse.error != QJsonParseError::NoError ||
                !doc.isObject())
            {
                print("[error] Invalid server response.");
                socket.abort();
                return;
            }

            auto m = doc.object();
            QString type = m["type"].toString();

            // Successful login
            if (type == "welcome")
            {
                joined = true;
                currentChannel.clear();

                timeout.stop();

                updateStatus();

                print("[connected] " + m["text"].toString());
            }

            // Online users
            else if (type == "users")
            {
                QStringList names;

                for (const auto& v : m["names"].toArray())
                {
                    names.append(v.toString());
                }

                print(
                    QString("[online: %1] %2")
                        .arg(names.size())
                        .arg(names.join(", "))
                );
            }

            // Available channels
            else if (type == "channels")
            {
                QJsonArray channels = m["channels"].toArray();

                print("----- AVAILABLE CHANNELS -----");

                if (channels.isEmpty())
                {
                    print("No channels available.");
                }

                for (const auto& value : channels)
                {
                    QJsonObject channel = value.toObject();

                    print(
                        "#" + channel["name"].toString()
                        + " | Members: "
                        + QString::number(channel["members"].toInt())
                    );
                }

                print("------------------------------");
            }

            // Channel successfully created
            else if (type == "channel_created")
            {
                print("[channel] " + m["text"].toString());
            }

            // Successfully joined a channel
            else if (type == "channel_joined")
            {
                currentChannel = m["name"].toString();

                updateStatus();

                print("[channel] " + m["text"].toString());
            }

            // Left a channel
            else if (type == "channel_left")
            {
                currentChannel.clear();

                updateStatus();

                print("[channel] " + m["text"].toString());
            }

            // Chat messages
            else if (type == "chat")
            {
                QString channel = m["channel"].toString();

                print(
                    "[" + channel + "] "
                    + m["name"].toString()
                    + "> "
                    + m["text"].toString()
                );
            }

            // Notices and errors
            else if (type == "notice" || type == "error")
            {
                print("[" + type + "] " + m["text"].toString());
            }
        }

        if (socket.bytesAvailable() >= 8192)
        {
            print("[error] Server frame too large.");
            socket.abort();
        }
    }

public:

    TerminalWindow()
    {
        setWindowTitle("Chat Room — Terminal");
        resize(860, 560);
        setMinimumSize(520, 320);

        setFont(QFontDatabase::systemFont(
            QFontDatabase::FixedFont
        ));

        setStyleSheet(
            "QWidget {background:#080a09;color:#b8efc5;font-size:14px;}"
            "QPlainTextEdit {border:0;padding:10px;"
            "selection-background-color:#245b38;}"
            "QLineEdit {border:1px solid #31563c;padding:12px;"
            "background:#0d120f;}"
            "QLabel {color:#73ac82;padding:6px;}"
        );

        auto* layout = new QVBoxLayout(this);

        status = new QLabel("OFFLINE", this);

        output = new QPlainTextEdit(this);
        output->setReadOnly(true);
        output->setMaximumBlockCount(2000);

        input = new QLineEdit(this);
        input->setMaxLength(2000);

        input->setPlaceholderText(
            "> Type /help or /connect 127.0.0.1 5050 Ryan"
        );

        layout->addWidget(status);
        layout->addWidget(output, 1);
        layout->addWidget(input);

        timeout.setSingleShot(true);
        socket.setReadBufferSize(16384);

        // Input handling
        connect(input, &QLineEdit::returnPressed,
                this, [this]
        {
            command();
        });

        // TCP connected
        connect(&socket, &QTcpSocket::connected,
                this, [this]
        {
            print("[system] TCP connected. Please log in or register.");

            timeout.stop();

            status->setText("CONNECTED / NOT AUTHENTICATED");
        });

        // Incoming messages
        connect(&socket, &QTcpSocket::readyRead,
                this, [this]
        {
            receive();
        });

        // Disconnection
        connect(&socket, &QTcpSocket::disconnected,
                this, [this]
        {
            joined = false;
            currentChannel.clear();

            timeout.stop();

            updateStatus();

            print("[system] Disconnected from server.");
        });

        // Connection errors
        connect(&socket, &QTcpSocket::errorOccurred,
                this, [this](QAbstractSocket::SocketError)
        {
            timeout.stop();

            joined = false;
            currentChannel.clear();

            updateStatus();

            print("[error] " + socket.errorString());

            socket.abort();
        });

        // Connection timeout
        connect(&timeout, &QTimer::timeout,
                this, [this]
        {
            print("[error] Connection timed out.");

            socket.abort();

            joined = false;
            currentChannel.clear();

            updateStatus();
        });

        help();

        input->setFocus();
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    TerminalWindow window;
    window.show();

    return app.exec();
}

