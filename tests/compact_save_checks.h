#pragma once
#include "elements/ElementCatalog.h"

static int checkCompactSave(LoadedProjects &projects)
{
    const int original=projects.currentProjectIndex(), count=projects.projectCount();
    projects.setCurrentProjectIndex(projects.addProject("Compact save"));
    CityLayoutView view;
    ProjectView editor;
    struct Cleanup {
        LoadedProjects &projects; int count, original;
        ~Cleanup() {
            while(projects.projectCount()>count) projects.removeProject(projects.projectCount()-1);
            projects.setCurrentProjectIndex(original);
        }
    } cleanup{projects,count,original};
    int index=0;
    for(const auto &type:ElementCatalog::instance().definitions()) {
        QPixmap artwork(type.resourcePath);
        QSize size=type.size;
        if(type.name=="BasePlate32x32Green") { artwork=QPixmap(1,1);artwork.fill(Qt::red);size=QSize(4,4); }
        auto *element=new CityLayoutElement(type.name,artwork,size,LegoGrid::pixelsPerStud,
            projects.currentProject()->zoomFactor,&view,type.resourcePath);
        element->rotateByDegrees(element->isTrack() ? 22.65329257274189 : 90.0);
        if(element->isFlexTrack()) element->setFlexBendDegrees(1.25);
        element->setLayoutPosition(QPointF(100.125+index*50,200.375+index*25));
        projects.cityLayouts().append(element);
        ++index;
    }
    const QJsonObject saved=view.savedLayout();
    QJsonArray placements=saved.value("elements").toArray();
    if(placements.size()!=int(ElementCatalog::instance().definitions().size())) return 100;
    for(const auto &value:placements) {
        const auto element=value.toObject();
        if(!ElementCatalog::instance().byName(element.value("name").toString())
            || element.size()!=(element.value("name")=="Flex Track" ? 5 : 4)
            || element.contains("imagePngBase64") || element.contains("resourcePath")
            || element.contains("widthStuds") || element.contains("heightStuds")) return 101;
    }
    // Supplied image data, resource paths, and dimensions must never override app definitions.
    for(int i=0;i<placements.size();++i) {
        auto element=placements[i].toObject();
        element.insert("imagePngBase64","invalid image");
        element.insert("resourcePath","/does/not/exist.png");
        element.insert("widthStuds",1);element.insert("heightStuds",1);
        placements[i]=element;
    }
    QJsonObject city=saved;city.insert("elements",placements);
    const QJsonArray outline{QJsonObject{{"x",0},{"y",0}},QJsonObject{{"x",500},{"y",0}},
        QJsonObject{{"x",500},{"y",300}},QJsonObject{{"x",0},{"y",300}}};
    const QJsonArray surfaces{QJsonObject{{"name","Desk"},{"outline",outline}}};
    const QJsonArray sides{QJsonObject{{"start",QJsonObject{{"x",0},{"y",0}}},
                                     {"end",QJsonObject{{"x",500},{"y",0}}}}};
    QTemporaryDir temporary;
    if(!temporary.isValid()) return 102;
    const QString path=temporary.filePath("compact.LegoLayout");
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return 103;
    file.write(QJsonDocument(QJsonObject{{"format","LegoCityLayout.TableDefinition"},{"version",2},
        {"layoutName","Reload compact"},{"surfaces",surfaces},{"openSides",sides},{"cityLayout",city}}).toJson());
    file.close();
    if(!editor.openLayout(path)) return 104;
    if(projects.cityLayouts().size()!=placements.size() || projects.tableDefinition().surfaces().size()!=1
        || projects.tableDefinition().openSides().size()!=1) return 105;
    for(int i=0;i<placements.size();++i) {
        const auto &type=ElementCatalog::instance().definitions()[i];
        const auto *element=projects.cityLayouts()[i];
        if(element->name()!=type.name || element->resourcePath()!=type.resourcePath || element->plateSize()!=type.size
            || element->sourcePixmap().toImage()!=QPixmap(type.resourcePath).toImage()
            || element->pos()!=QPoint(100+i*50,200+i*25)
            || !near(element->rotationDegrees(),element->isTrack() ? 22.65329257274189 : 90.0)
            || !near(element->flexBendDegrees(),type.name=="Flex Track" ? 1.25 : 0.0)) return 106;
    }
    if(view.savedLayout()!=saved) return 107;
    qInfo()<<"PASS: compact manifests; all catalog types reload current artwork/dimensions; placement and table geometry preserved";
    return 0;
}
