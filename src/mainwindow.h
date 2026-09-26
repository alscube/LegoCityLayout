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
    void SetupMainMenu( );
    void CreateSplitterView( );

    void defineTableOutline();
    void showCityLayout();
    void showSettings();

    QSplitter *m_splitter = nullptr;
    ProjectView *m_projectView = nullptr;
};
