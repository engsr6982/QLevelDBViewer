#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "iostream"
#include "nlohmann/json.hpp"
#include "qaction.h"
#include "qdialog.h"
#include "qlistwidget.h"
#include "qmessagebox.h"
#include "qnamespace.h"
#include "qobject.h"
#include "qobjectdefs.h"
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
using json   = nlohmann::json;
namespace fs = std::filesystem;
string x_str(QString const& qstr) { return string((const char*)qstr.toUtf8()); }
void   x_parse(string& str, int indent = 4) {
    try {
        str = json::parse(str).dump(indent);
    } catch (...) {}
}


MainWindow::~MainWindow() { delete ui; }
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    int const curHeight = height();
    int const curWidth  = width();
    setMaximumHeight(curHeight);
    setMinimumHeight(curHeight);
    setMaximumWidth(curWidth);
    setMinimumWidth(curWidth);

    refreshButton(); // 刷新按钮状态

    // 链接列表框，动态更新Value
    connect(ui->mKeyList, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        if (!mDB) return;
        refreshValueText(item->text());
    });
    connect(ui->mKeyList, &QListWidget::itemSelectionChanged, [this]() {
        if (!mDB) return;
        auto it = ui->mKeyList->currentItem();
        if (!it) return;
        refreshValueText(it->text());
    });
}


void MainWindow::refreshValueText(QString const& qstr) {
    string key   = x_str(qstr);
    auto   value = mDB->get(key);
    if (value) {
        string val = *value;

        if (ui->mSwitchViewer->text() == "JSON视图") {
            x_parse(val);
        }

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
void MainWindow::refreshButton() {
    bool const isOpen = mDB != nullptr;
    ui->mOpenDB->setDisabled(isOpen);        // 已打开数据库，禁用打开按钮
    ui->mCloseDB->setDisabled(!isOpen);      // 未打开数据库，禁用关闭按钮
    ui->mInsertKV->setDisabled(!isOpen);     // 未打开数据库，禁用插入按钮
    ui->mDeleteKV->setDisabled(!isOpen);     // 未打开数据库，禁用删除按钮
    ui->mReWriteKey->setDisabled(!isOpen);   // 未打开数据库，禁用重写Key按钮
    ui->mReWriteValue->setDisabled(!isOpen); // 未打开数据库，禁用重写Value按钮
}


void MainWindow::on_mOpenDB_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "打开数据库目录", "./");
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "QLevelDBViewer", "请选择正确的数据库目录！");
        return;
    }

    string   db_path = x_str(dir);
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
        refreshButton();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "QLevelDBViewer", "打开数据库失败！\n" + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::warning(this, "QLevelDBViewer", "打开数据库失败！");
    }
}
void MainWindow::on_mCloseDB_clicked() {
    mDB.reset();             // 释放数据库资源
    ui->mKeyList->clear();   // 清空 KeyList
    ui->mValueText->clear(); // 清空 ValueText
    refreshButton();         // 刷新按钮状态
}


void MainWindow::on_mInsertKV_clicked() {
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

    // 增强体验，自动滚动到新增的行
    auto its = ui->mKeyList->findItems(qkey, Qt::MatchExactly);
    if (its.size() == 1) {
        QListWidgetItem* fir = its.first();
        ui->mKeyList->scrollToItem(fir);   // 滚动到指定行
        fir->setSelected(true);            // 选中指定行
        ui->mKeyList->setCurrentItem(fir); // 设置当前行
        ui->mKeyList->itemClicked(fir);    // 触发选中事件
    }
    QMessageBox::information(this, "QLevelDBViewer", "添加成功！");
}
void MainWindow::on_mDeleteKV_clicked() {
    int row = ui->mKeyList->currentRow();
    if (row == -1) {
        QMessageBox::warning(this, "QLevelDBViewer", "未选择 Key！");
        return;
    }
    auto   it  = ui->mKeyList->takeItem(row);
    string key = x_str(it->text());
    mDB->del(key);
    delete it;
    ui->mValueText->clear();        // 清空 ValueText
    ui->mKeyList->clearSelection(); // 清空选中
    QMessageBox::information(this, "QLevelDBViewer", "删除成功！");
}
void MainWindow::on_mReWriteKey_clicked() {
    int row = ui->mKeyList->currentRow();
    if (row == -1) {
        QMessageBox::warning(this, "QLevelDBViewer", "未选择 Key！");
        return;
    }
    auto   it  = ui->mKeyList->item(row);
    string key = x_str(it->text());

    QString qkey = QInputDialog::getText(this, "QLevelDBViewer", "修改 Key:", QLineEdit::Normal, it->text());
    if (qkey.isEmpty()) {
        QMessageBox::warning(this, "QLevelDBViewer", "Key 不能为空！");
        return;
    }

    string oldData = *mDB->get(key); // 获取旧数据
    mDB->del(key);                   // 删除旧数据
    mDB->set(x_str(qkey), oldData);  // 写入新数据
    refreshUI();                     // 刷新UI

    // 增强体验，自动滚动到新增的行
    auto its = ui->mKeyList->findItems(qkey, Qt::MatchExactly);
    if (its.size() == 1) {
        QListWidgetItem* fir = its.first();
        ui->mKeyList->scrollToItem(fir);   // 滚动到指定行
        fir->setSelected(true);            // 选中指定行
        ui->mKeyList->setCurrentItem(fir); // 设置当前行
        ui->mKeyList->itemClicked(fir);    // 触发选中事件
    }
    QMessageBox::information(this, "QLevelDBViewer", "重写成功！");
}
void MainWindow::on_mReWriteValue_clicked() {
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

    // 增强体验，自动滚动到新增的行
    auto its = ui->mKeyList->findItems(qvalue, Qt::MatchExactly);
    if (its.size() == 1) {
        QListWidgetItem* fir = its.first();
        ui->mKeyList->scrollToItem(fir);   // 滚动到指定行
        fir->setSelected(true);            // 选中指定行
        ui->mKeyList->setCurrentItem(fir); // 设置当前行
        ui->mKeyList->itemClicked(fir);    // 触发选中事件
    }
    QMessageBox::information(this, "QLevelDBViewer", "重写成功！");
}


void MainWindow::on_mSwitchViewer_clicked() {
    if (ui->mSwitchViewer->text() == "原始视图") {
        ui->mSwitchViewer->setText(QString::fromStdString("JSON视图"));
    } else if (ui->mSwitchViewer->text() == "JSON视图") {
        ui->mSwitchViewer->setText(QString::fromStdString("原始视图"));
    }

    if (!mDB) return;
    auto it = ui->mKeyList->currentItem();
    if (!it) return;
    refreshValueText(it->text());
}
void MainWindow::on_mBugReport_clicked() {
    QDesktopServices::openUrl(QUrl("https://github.com/engsr6982/QLevelDBViewer/issues"));
}
