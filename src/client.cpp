
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

class TerminalWindow : public QWidget {
  QTcpSocket socket;
  QTimer timeout;
  QPlainTextEdit *output;
  QLineEdit *input;
  QLabel *status;
  QString username;
  bool joined = false;

  void print(const QString &s) {
    output->appendPlainText(s);
  }

  void send(const QJsonObject &m) {
    if (socket.bytesToWrite() > 65536) {
      print("[error] Connection is backed up; try again shortly.");
      return;
    }

    socket.write(
        QJsonDocument(m).toJson(QJsonDocument::Compact) + '\n'
    );
  }

  void help() {
    print(
        "CHAT ROOM / starter\n"
        "  /connect HOST PORT NAME  - connect to a server\n"
        "  /register EMAIL PASSWORD - create an account\n"
        "  /login PASSWORD          - log in to your account\n"
        "  /users                   - list online users\n"
        "  /disconnect              - leave the server\n"
        "  /clear   /help   /quit\n\n"
        "Example:\n"
        "  /connect 127.0.0.1 5050 Ryan\n"
        "  /register ryan@example.com TestPass123\n"
        "  /login TestPass123\n\n"
        "After login, type a message and press Enter.\n"
    );
  }

  void command() {
    const QString text = input->text().trimmed();
    input->clear();

    if (text.isEmpty()) return;

    if (text == "/clear") {
      output->clear();
      return;
    }

    if (text == "/help") {
      help();
      return;
    }

    if (text == "/quit") {
      close();
      return;
    }

    if (text == "/disconnect") {
      timeout.stop();
      socket.abort();
      joined = false;
      status->setText("OFFLINE");
      return;
    }

    const auto parts =
        text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (parts[0] == "/connect") {
      if (socket.state() != QAbstractSocket::UnconnectedState) {
        print("[info] Use /disconnect before connecting again.");
        return;
      }

      bool ok = false;
      int port = parts.size() == 4 ? parts[2].toInt(&ok) : 0;

      if (!ok || port < 1 || port > 65535 ||
          !QRegularExpression("^[A-Za-z0-9_-]{1,24}$")
               .match(parts.value(3)).hasMatch()) {
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
          parts[1], static_cast<quint16>(port)
      );

      return;
    }

    // Register an account for the username chosen with /connect.
    if (parts[0] == "/register") {
      if (socket.state() != QAbstractSocket::ConnectedState) {
        print("[error] Connect to a server first.");
        return;
      }

      if (joined) {
        print("[error] Disconnect before registering another account.");
        return;
      }

      if (parts.size() != 3) {
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

    // Log in using the username chosen with /connect.
    if (parts[0] == "/login") {
      if (socket.state() != QAbstractSocket::ConnectedState) {
        print("[error] Connect to a server first.");
        return;
      }

      if (joined) {
        print("[info] You are already logged in.");
        return;
      }

      if (parts.size() != 2) {
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

    if (text.startsWith('/') && text != "/users") {
      print("Unknown command. Type /help.");
      return;
    }

    if (!joined) {
      print("[info] Log in first. Type /help.");
      return;
    }

    if (text == "/users") {
      send({{"type", "users"}});
      return;
    }

    send({{"type", "chat"}, {"text", text}});
  }

  void receive() {
    while (socket.canReadLine()) {
      auto line = socket.readLine(8193);

      if (line.size() > 8192 || !line.endsWith('\n')) {
        print("[error] Invalid server frame.");
        socket.abort();
        return;
      }

      QJsonParseError parse;
      auto doc = QJsonDocument::fromJson(line, &parse);

      if (parse.error != QJsonParseError::NoError ||
          !doc.isObject()) {
        print("[error] Invalid server response.");
        socket.abort();
        return;
      }

      auto m = doc.object();
      QString type = m["type"].toString();

      if (type == "welcome") {
        joined = true;
        timeout.stop();

        status->setText(
            "ONLINE / lobby / " + m["name"].toString()
        );

        print("[connected] " + m["text"].toString());
      } else if (type == "users") {
        QStringList names;

        for (const auto &v : m["names"].toArray()) {
          names.append(v.toString());
        }

        print(
            QString("[online: %1] %2")
                .arg(names.size())
                .arg(names.join(", "))
        );
      } else if (type == "chat") {
        print(m["name"].toString() + "> " +
              m["text"].toString());
      } else if (type == "notice" || type == "error") {
        print("[" + type + "] " + m["text"].toString());
      }
    }

    if (socket.bytesAvailable() >= 8192) {
      print("[error] Server frame too large.");
      socket.abort();
    }
  }

public:
  TerminalWindow() {
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

    auto *layout = new QVBoxLayout(this);

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

    connect(input, &QLineEdit::returnPressed,
            this, [this] { command(); });

    connect(&socket, &QTcpSocket::connected, this, [this] {
      print("[system] TCP connected. Please log in or register.");
      timeout.stop();
      status->setText("CONNECTED / NOT AUTHENTICATED");
    });

    connect(&socket, &QTcpSocket::readyRead,
            this, [this] { receive(); });

    connect(&socket, &QTcpSocket::disconnected, this, [this] {
      joined = false;
      timeout.stop();
      status->setText("OFFLINE");
      print("[system] Disconnected from server.");
    });

    connect(&socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
      timeout.stop();
      joined = false;
      status->setText("OFFLINE");
      print("[error] " + socket.errorString());
      socket.abort();
    });

    connect(&timeout, &QTimer::timeout, this, [this] {
      print("[error] Connection timed out.");
      socket.abort();
      status->setText("OFFLINE");
      joined = false;
    });

    help();
    input->setFocus();
  }
};

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  TerminalWindow window;
  window.show();

  return app.exec();
}
