#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "iostream"
#include "nlohmann/json.hpp"
#include "qaction.h"
#include "qdialog.h"
#include "qlistwidget.h"
#include "qmessagebox.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>


// Tools functions
using string = std::string;
namespace fs = std::filesystem;
using json   = nlohmann::json;
string x_str(QString const& qstr) { return string((const char*)qstr.toUtf8()); }
void   x_parse(string& str, int indent = 4) {
    try {
        str = json::parse(str).dump(indent);
    } catch (...) {}
}


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    int hei = height();
    int wid = width();
    setMaximumHeight(hei);
    setMinimumHeight(hei);
    setMaximumWidth(wid);
    setMinimumWidth(wid);


    // Menu Bar File 菜单栏操作
    connect(ui->mMenuBar_File_Exit, &QAction::triggered, [this]() { this->close(); });
    connect(ui->mMenuBar_File_OpenDB, &QAction::triggered, this, [this]() { this->openDB(); });
    connect(ui->mMenuBar_File_CloseDB, &QAction::triggered, [this]() {
        if (!mDB) {
            QMessageBox::warning(this, "QLevelDBViewer", "数据库未打开！");
            return;
        }
        mDB.reset();             // 释放数据库资源
        ui->mKeyList->clear();   // 清空 KeyList
        ui->mValueText->clear(); // 清空 ValueText
    });


    // Menu Bar Edit 菜单栏操作
    connect(ui->mMenuBar_Edit_AddKeyValue, &QAction::triggered, this, [this]() {
        if (!mDB) {
            QMessageBox::warning(this, "QLevelDBViewer", "数据库未打开！");
            return;
        }
        QString qkey   = QInputDialog::getText(this, "QLevelDBViewer", "输入 Key:");
        QString qvalue = QInputDialog::getMultiLineText(this, "QLevelDBViewer", "输入 Value:");
        if (qkey.isEmpty() || qvalue.isEmpty()) {
            QMessageBox::warning(this, "QLevelDBViewer", "Key 或 Value 不能为空！");
            return;
        };

        string key   = x_str(qkey);
        string value = x_str(qvalue);

        if (mDB->has(key)) {
            auto q = QMessageBox::question(
                this,
                "QLevelDBViewer",
                "检测到输入的 Key 已存在，是否覆盖？",
                QMessageBox::Yes | QMessageBox::No
            );
            if (q == QMessageBox::No) return;

            mDB->set(key, value); // 覆盖
            refreshUI();          // 刷新UI
        } else {
            mDB->set(key, value);        // 新增
            ui->mKeyList->addItem(qkey); // 添加到 KeyList
        }

        // 滚动列表到新增的数据
        auto its = ui->mKeyList->findItems(qkey, Qt::MatchExactly);
        if (!its.isEmpty()) {
            int  row = ui->mKeyList->row(its.first());
            auto i   = ui->mKeyList->item(row);
            ui->mKeyList->scrollToItem(i);
            i->setSelected(true);
        }
        QMessageBox::information(this, "QLevelDBViewer", "添加成功！");
    });
    connect(ui->mMenuBar_Edit_DeleteKeyValue, &QAction::triggered, this, [this]() {
        if (!mDB) {
            QMessageBox::warning(this, "QLevelDBViewer", "数据库未打开！");
            return;
        }
        int row = ui->mKeyList->currentRow();
        if (row == -1) {
            QMessageBox::warning(this, "QLevelDBViewer", "未选择 Key！");
            return;
        }
        auto   it  = ui->mKeyList->takeItem(row);
        string key = x_str(it->text());
        mDB->del(key);
        delete it;
        ui->mValueText->clear();
        QMessageBox::information(this, "QLevelDBViewer", "删除成功！");
    });
    connect(ui->mMenuBar_Edit_ReWriteKey, &QAction::triggered, this, [this]() {
        if (!mDB) {
            QMessageBox::warning(this, "QLevelDBViewer", "数据库未打开！");
            return;
        }

        int row = ui->mKeyList->currentRow();
        if (row == -1) {
            QMessageBox::warning(this, "QLevelDBViewer", "未选择 Key！");
            return;
        }
        auto   it  = ui->mKeyList->item(row);
        string key = x_str(it->text());

        QString qkey = QInputDialog::getText(this, "QLevelDBViewer", "输入 Key:", QLineEdit::Normal, it->text());
        if (qkey.isEmpty()) {
            QMessageBox::warning(this, "QLevelDBViewer", "Key 不能为空！");
            return;
        }

        string oldData = *mDB->get(key); // 获取旧数据
        mDB->del(key);                   // 删除旧数据
        mDB->set(x_str(qkey), oldData);  // 写入新数据
        refreshUI();                     // 刷新UI
        QMessageBox::information(this, "QLevelDBViewer", "重写成功！");
    });
    connect(ui->mMenuBar_Edit_ReWriteValue, &QAction::triggered, this, [this]() {
        if (!mDB) {
            QMessageBox::warning(this, "QLevelDBViewer", "数据库未打开！");
            return;
        }
        int row = ui->mKeyList->currentRow();
        if (row == -1) {
            QMessageBox::warning(this, "QLevelDBViewer", "未选择 Key！");
            return;
        }
        auto   it    = ui->mKeyList->item(row);
        string key   = x_str(it->text());
        string value = *mDB->get(key);
        x_parse(value); // 格式化便于编辑

        QString qvalue =
            QInputDialog::getMultiLineText(this, "QLevelDBViewer", "输入 Value:", QString::fromStdString(value));
        if (qvalue.isEmpty()) {
            QMessageBox::warning(this, "QLevelDBViewer", "Value 不能为空！");
            return;
        }

        value = x_str(qvalue);
        x_parse(value, -1); // 去格式化

        mDB->set(key, value);
        refreshUI(); // 刷新UI
    });


    // Menu Bar About 菜单栏操作
    connect(ui->mMenuBar_About_OpenRepo, &QAction::triggered, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/engsr6982/QLevelDBViewer"));
    });
    connect(ui->mMenuBar_About_OpenIssuse, &QAction::triggered, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/engsr6982/QLevelDBViewer/issues"));
    });


    // KeyListWidget
    connect(ui->mKeyList, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        if (!mDB) return;
        handleList(item->text());
    });
    connect(ui->mKeyList, &QListWidget::itemSelectionChanged, [this]() {
        if (!mDB) return;
        auto it = ui->mKeyList->currentItem();
        if (!it) return;
        handleList(it->text());
    });
}

MainWindow::~MainWindow() { delete ui; }
void MainWindow::handleList(QString const& qstr) {
    string key   = x_str(qstr);
    auto   value = mDB->get(key);
    if (value) {
        string val = *value;
        x_parse(val);
        ui->mValueText->setText(QString::fromStdString(val));
    }
}

void MainWindow::refreshUI() {
    if (!mDB) return;
    ui->mKeyList->clear();
    ui->mValueText->clear();

    mDB->iter([this](std::string_view key, std::string_view value) {
        ui->mKeyList->addItem(QString::fromStdString(string(key)));
        return true;
    });
}

void MainWindow::openDB() {
    QString dir = QFileDialog::getExistingDirectory(this, "打开数据库目录", "./");
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "QLevelDBViewer", "未选择目录！");
        return;
    }
    if (mDB) {
        QMessageBox::warning(this, "QLevelDBViewer", "数据库已打开！");
        return;
    }
    string   db_path = x_str(dir.toUtf8());
    fs::path db_dir(db_path);

    // 检查LevelDB标志性的文件
    if (!fs::exists(db_dir / "LOCK") && !fs::exists(db_dir / "CURRENT")) {
        auto v = QMessageBox::question(
            this,
            QString("QLevelDBViewer"),
            QString("检测到目录：\n%1\n非LevelDB数据库，是否创建?").arg(dir),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (QMessageBox::No == v) return;
    }

    try {
        mDB = std::make_unique<KeyValueDB>(db_path);

        refreshUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "QLevelDBViewer", "打开数据库失败！\n" + QString::fromStdString(e.what()));
        return;
    } catch (...) {
        QMessageBox::warning(this, "QLevelDBViewer", "打开数据库失败！");
        return;
    }
}
