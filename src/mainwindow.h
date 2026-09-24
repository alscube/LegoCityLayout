#pragma once

#include <QMainWindow>

class QSplitter;
class CityLayoutView;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createNewLayout();
    void defineTableOutline();

    QSplitter *m_splitter = nullptr;
    CityLayoutView *m_cityLayoutView = nullptr;
};
