#pragma once

#include <QMainWindow>

class QSplitter;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QSplitter *m_splitter = nullptr;
};
