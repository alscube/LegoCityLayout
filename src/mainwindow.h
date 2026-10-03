#pragma once

#include <QMainWindow>

class QSplitter;
class LayoutElementsView;
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

    QSplitter *_splitter = nullptr;
    LayoutElementsView* _layoutElementsView = nullptr;
    ProjectView* _projectView = nullptr;
};
