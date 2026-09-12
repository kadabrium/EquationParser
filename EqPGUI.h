#pragma once
#include <QMainWindow>
#include <QString>
#include <string_view>
#include "output.h"

// Every widget below is held by pointer, so a forward declaration is enough and
// the full Qt headers stay in EqPGUI.cpp.  QT_BEGIN_NAMESPACE is not decoration:
// Qt can be built with QT_NAMESPACE set, in which case these classes are not at
// global scope and a bare `class QLabel;` would declare a different type.
QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

// No `using namespace EqP;` here.  A using-directive in a header leaks into
// every TU that includes it, and there is no way for those TUs to opt out.
// The .cpp opens the namespace for itself instead.
class EqPGUI: public QMainWindow {
Q_OBJECT

public:
  explicit EqPGUI(EqP::out::Renderer renderer, QWidget* parent = nullptr);

private:
  EqP::out::Renderer renderer;
  // global access widgets
  QPlainTextEdit* inputBox = nullptr;
  QLineEdit* outputBox = nullptr;
  QLabel* errorMsg = nullptr;
  QNetworkAccessManager* manager = nullptr;
  QLabel* imgSlot = nullptr;

  // Deliberately not slots.  Every connect() in the .cpp uses the
  // pointer-to-member form, which binds at compile time and accepts any member
  // function.  `slots:` is only required when something resolves the method by
  // name at runtime through the meta-object: the string-based SLOT() syntax,
  // QMetaObject::invokeMethod, QML, QtTest's qExec, or a QtDBus adaptor.
  static QString svQstr(std::string_view sv);
  void loadSettings();
  void saveSettings() const;
  void setupUI();
  void showFileDlg();
  void showSettingsDlg();
  void changeSetting(const QString& settingCmd);
  void renderStr();
  void reqRenderImg();
  void renderReply(QNetworkReply* reply);
  void saveImg();
  void copyImg();
};
