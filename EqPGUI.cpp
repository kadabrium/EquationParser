#include "EqPGUI.h"

#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "ctrl.h"
using namespace EqP;


EqPGUI::EqPGUI(out::Renderer r, QWidget* parent):
  QMainWindow(parent),
  renderer{r} {
  loadSettings();
  setupUI();
}

QString EqPGUI::svQstr(std::string_view sv) {
  return QString::fromUtf8(sv.data(), qsizetype(sv.size()));
}

void EqPGUI::setupUI() {
  setWindowTitle("EquationParser GUI");
  resize(640, 480);

  // menu
  // file: batch convert, save image
  auto* fileMnu = menuBar()->addMenu("&File");
  fileMnu->addAction("&Batch convert…",   
    QKeySequence::Open, this, &EqPGUI::showFileDlg);
  fileMnu->addAction("&Save rendered image…",   
    QKeySequence::Save, this, &EqPGUI::saveImg);
  
  auto* settingMnu = menuBar()->addMenu("&Settings");
  settingMnu->addAction("&Settings…",   
    QKeySequence("F5"), this, &EqPGUI::showSettingsDlg);
  
  // main
  auto* splitter = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(splitter);

  // left: static operation notation list
  auto* leftWidget = new QWidget;
  auto* leftBox = new QVBoxLayout(leftWidget);
  leftBox->setContentsMargins(2, 2, 2, 2);
 // leftBox->setSpacing(2);
  QString opTable;
  for (int i = 0; i < ctrl::numBuiltinOps; i++) {
    opTable += svQstr(ctrl::builtinOpNotation[i]);
    opTable += "\n\n";
   // leftBox->addWidget(new QLabel(svQstr(ctrl::builtinOpNotation[i]), this));
  }
  leftBox->addWidget(new QLabel(opTable, this));
  splitter->addWidget(leftWidget);

  // right: input box, convert button, output box, error panel,
  // copy button, render button, slot for rendered image
  auto* rightWidget = new QWidget;
  auto rightBox = new QVBoxLayout(rightWidget);

  this->inputBox = new QPlainTextEdit(this);
  inputBox->setPlaceholderText("Enter expression...");
  inputBox->setStyleSheet("font-size: 20px");
  inputBox->setFixedWidth(350);
  inputBox->setFixedHeight(175);

  this->errorMsg = new QLabel(this);
  errorMsg->setStyleSheet("color: #b00020;");
  errorMsg->setWordWrap(true);
  errorMsg->setFixedHeight(25);

  auto* convertBtn = new QPushButton("Convert");
  connect(convertBtn, &QPushButton::clicked, this, &EqPGUI::renderStr);

  this->outputBox = new QLineEdit(this);
  outputBox->setStyleSheet("font-size: 20px");
  outputBox->setFixedWidth(350);

  auto* copyBtn = new QPushButton("Copy");
  connect(copyBtn, &QPushButton::clicked, this, [this](){
    ctrl::windowsClipboard(this->outputBox->text().toStdString());
  });
  auto* renderBtn = new QPushButton("Render (Powered by codecogs.com)");
  this->imgSlot = new QLabel();
  connect(renderBtn, &QPushButton::clicked, this, &EqPGUI::reqRenderImg);

  // rightWidget already has rightBox installed, 
  // addLayout below is what gives this row an owner.
  auto* imgButtonRow = new QHBoxLayout();
  auto* copyImgBtn = new QPushButton("Copy Image");
  auto* saveImgBtn = new QPushButton("Save Image");
  connect(copyImgBtn, &QPushButton::clicked, this, &EqPGUI::copyImg);
  connect(saveImgBtn, &QPushButton::clicked, this, &EqPGUI::saveImg);
  imgButtonRow->addWidget(copyImgBtn);
  imgButtonRow->addWidget(saveImgBtn);

  rightBox->addWidget(inputBox);
  rightBox->addWidget(convertBtn);
  rightBox->addWidget(errorMsg);
  rightBox->addWidget(outputBox);
  rightBox->addWidget(copyBtn);
  rightBox->addWidget(renderBtn);
  rightBox->addWidget(imgSlot);
  rightBox->addLayout(imgButtonRow);
  splitter->addWidget(rightWidget);

  splitter->setSizes({480, 300});
 // splitter->show();

}

void EqPGUI::saveSettings() const {
  QSettings settings;
  for (std::size_t i = 0; i < ctrl::numOptions; i++) {
    settings.setValue(
      svQstr(ctrl::optionCodeNames[i]),
      svQstr(ctrl::getOptionStatus(this->renderer, i)));
  }
}

void EqPGUI::loadSettings() {
  QSettings settings;
  for (std::size_t i = 0; i < ctrl::numOptions; i++) {
    const QString saved = settings.value(svQstr(ctrl::optionCodeNames[i])).toString();
    if (saved.isEmpty()) { continue; }
    ctrl::setOption(
      (svQstr(ctrl::optionCodeNames[i]) + "::" + saved).toStdString(),
      this->renderer);
  }
}

void EqPGUI::showSettingsDlg() {
  QDialog settingDlg(this);
  settingDlg.setWindowTitle("EquationParser settings");
  settingDlg.resize(460, 460);

  auto* box = new QVBoxLayout(&settingDlg);
  // one line of desc followed by one row of a group of exclusive choices
  for (std::size_t i = 0; i < ctrl::numOptions; i++) {
    box->addWidget(new QLabel(svQstr(ctrl::optionDesc[i])));

    auto* choicesRow = new QHBoxLayout();
    box->addLayout(choicesRow);
    auto* typeGroup = new QButtonGroup(&settingDlg);
    std::span<const std::string_view> choices = ctrl::optionChoices[i];
    for (size_t j = 0; j < choices.size(); j++) {
      auto* optionRadio = new QRadioButton(svQstr(choices[j]));
      typeGroup->addButton(optionRadio, (int)j);   
      choicesRow->addWidget(optionRadio);
      // check the choice whose index is currently selected in the controller
      if (j == ctrl::optionIndex(this->renderer, i)) {
        optionRadio->setChecked(true);
      }
    }
    // change setting by sending text command to original controller
    connect(typeGroup, &QButtonGroup::idClicked, this, 
      [this, i, typeGroup](int id) {
        changeSetting(svQstr(ctrl::optionCodeNames[i]) + "::" + svQstr(ctrl::optionChoices[i][id]));
        // re-request the new selected choice after the change
        // from the controller and check that button
        if (auto* after = typeGroup->button(int(ctrl::optionIndex(this->renderer, i)))) {
          after->setChecked(true);
        }
    });
  }
  QDialogButtonBox* OKButton = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  connect(OKButton, &QDialogButtonBox::accepted, &settingDlg, &QDialog::accept);
  box->addWidget(OKButton);
  settingDlg.exec();
}

void EqPGUI::changeSetting(const QString& settingCmd) {
  ctrl::setOption(settingCmd.toStdString(), this->renderer);
  saveSettings();
}


void EqPGUI::renderStr() {
  const QString input = this->inputBox->toPlainText();
  ctrl::Outcome res = ctrl::render(input.toStdString(), this->renderer);
  if (res.ok()) {
    this->outputBox->setText(QString::fromStdString(res.latex));
    this->errorMsg->clear();
    if (this->renderer.opt.autoCopy) {
      ctrl::windowsClipboard(this->outputBox->text().toStdString());
    }
  }
  else {
    this->errorMsg->setText(QString::fromStdString(res.message));
    // highlight location of error in source input
    if (res.where) {
      int begin = res.where->begin; int end = res.where->end;
      this->inputBox->setFocus();  
      auto cursor = this->inputBox->textCursor();
      /*this->inputBox->setSelection(
        (begin == input.size()) ? begin-1 : begin, 
        (end <= begin) ? 1 : end-begin);*/
      cursor.setPosition((begin == input.size()) ? begin-1 : begin);
      cursor.setPosition((end <= begin) ? 1 : end-begin);
      this->inputBox->setTextCursor(cursor);
    }
  }
  
}

void EqPGUI::showFileDlg() {
  QDialog batchDlg(this);
  batchDlg.setWindowTitle("Select file");
  batchDlg.resize(460, 360);
  auto* box = new QVBoxLayout(&batchDlg);
  
  // input file selector and button
  auto* sourcePathLine = new QLineEdit(&batchDlg);
  auto* savePathLine = new QLineEdit(&batchDlg); // forward declared so default can access
  auto* sourcePathBtn = new QPushButton("Browse…", &batchDlg);
  connect(sourcePathBtn, &QPushButton::clicked, this, [this, sourcePathLine, savePathLine] {
    QString sourceFile = QFileDialog::getOpenFileName(this, "Select input file", QDir::homePath());
    if (!sourceFile.isEmpty()) {
      sourcePathLine->setText(sourceFile);
      // display (source)_converted as default save name
      savePathLine->setText(sourceFile + "_converted.txt");
    }
  });
  box->addWidget(sourcePathLine); box->addWidget(sourcePathBtn);
  // save path selector and button
  auto* savePathBtn = new QPushButton("Browse…", &batchDlg);
  connect(savePathBtn, &QPushButton::clicked, this, [this, savePathLine] {
    // load same folder and default name as above in dialog
    QString saveFileName = QFileDialog::getSaveFileName(this, "Select save path", savePathLine->text());
    if (!saveFileName.isEmpty()) {
      savePathLine->setText(saveFileName);
    }
  });
  box->addWidget(savePathLine); box->addWidget(savePathBtn);
  // convert button
  auto* convertBtn = new QPushButton("Convert", &batchDlg);
  connect(convertBtn, &QPushButton::clicked, this, [this, sourcePathLine, savePathLine] {
    const QString sourcePath = sourcePathLine->text();
    const QString savePath = savePathLine->text();
    if (!sourcePath.isEmpty() && !savePath.isEmpty()) {
      ctrl::Outcome res = ctrl::batchProcess(sourcePath.toStdString(), savePath.toStdString(), this->renderer);
      QMessageBox::information(this, "Batch convert", 
        QString("Batch conversion complete.\n") + QString::fromStdString(res.latex));
    }
  });
  box->addWidget(convertBtn);
  batchDlg.exec();
}

void EqPGUI::reqRenderImg() {
  QString latexStr = QUrl::toPercentEncoding(this->outputBox->text());
  if (!this->manager) this->manager = new QNetworkAccessManager(this);
  QUrl urlInput(QString("https://latex.codecogs.com/svg.image?") + latexStr);
  QNetworkRequest request(urlInput);
  QNetworkReply* reply = manager->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    renderReply(reply);
  });

}

void EqPGUI::renderReply(QNetworkReply* reply) {
 // QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply) {
    if (reply->error() == QNetworkReply::NoError) {
      QImage img;
      img.loadFromData(reply->readAll());
      this->imgSlot->setPixmap(QPixmap::fromImage(img));
    }
    else {
      this->errorMsg->setText("Network Error: Error connecting to rendering service");
    }
    reply->deleteLater();
  }
  else {
    this->errorMsg->setText("Network Error: Error connecting to rendering service");
  }
}

void EqPGUI::saveImg() {
  if (this->imgSlot && !this->imgSlot->pixmap().isNull()) {
    QString saveFileName = QFileDialog::getSaveFileName(this, "Save rendered image", QDir::homePath(), 
      "Images (*.png *.jpg)");
    if (!saveFileName.isEmpty()) {
      this->imgSlot->pixmap().save(saveFileName);
    }
  }
  else {
    QMessageBox::warning(this, "Save Image", "Please render an equation first.");
  }
}

void EqPGUI::copyImg() {
  if (this->imgSlot && !this->imgSlot->pixmap().isNull()) {
    QApplication::clipboard()->setPixmap(this->imgSlot->pixmap());
  }
  else {
    QMessageBox::warning(this, "Copy Image", "Please render an equation first.");
  }
}


int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setOrganizationName("EquationParser");
  app.setApplicationName("EquationParser GUI");

  //out::Renderer r{Options{}};
  EqPGUI gui(out::Renderer{Options{}});
  gui.show();
  return app.exec();
}
