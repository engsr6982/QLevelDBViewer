#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "db/KeyValueDB.h"
#include <QMainWindow>
#include <memory>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    std::unique_ptr<KeyValueDB> mDB;

    void refreshUI();
    void refreshButton();
    void refreshValueText(QString const& str);

private slots:
    // mFileBox
    void on_mOpenDB_clicked();
    void on_mCloseDB_clicked();

    // mActionBox
    void on_mInsertKV_clicked();
    void on_mDeleteKV_clicked();
    void on_mReWriteKey_clicked();
    void on_mReWriteValue_clicked();

    // mOtherBox
    void on_mSwitchViewer_clicked();
    void on_mBugReport_clicked();

private:
    Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H
