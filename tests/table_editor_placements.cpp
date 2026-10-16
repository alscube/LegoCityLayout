#include "LoadedProjects.h"
#include "CityLayoutView.h"
#include "elements/CityLayoutElement.h"
#include "LegoGrid.h"
#include <QApplication>
#include <QDebug>
#include "table_editor_placement_checks.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    return checkTableEditorPlacements(LoadedProjects::instance());
}
