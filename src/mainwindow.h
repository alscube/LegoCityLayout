#pragma once

#include <QMainWindow>

class QSplitter;
class ProjectView;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void defineTableOutline();
    void showCityLayout();

    QSplitter *m_splitter = nullptr;
    ProjectView *m_projectView = nullptr;
};
