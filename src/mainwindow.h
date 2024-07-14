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

    void openDB();
    void refreshUI();

    void handleList(QString const& str);

private:
    Ui::MainWindow* ui;
};
#endif // MAINWINDOW_H
