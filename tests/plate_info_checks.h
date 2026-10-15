#pragma once

#include <QDir>

static int checkPlateInfo()
{
    static_assert(!std::is_default_constructible_v<ElementCatalog>);
    static_assert(!std::is_copy_constructible_v<ElementCatalog>);
    static_assert(!std::is_copy_assignable_v<ElementCatalog>);
    static_assert(!std::is_move_constructible_v<ElementCatalog>);
    static_assert(!std::is_move_assignable_v<ElementCatalog>);
    auto &catalog=ElementCatalog::instance();
    if (&catalog!=&ElementCatalog::instance()
        || &catalog.loadPlates()!=&ElementCatalog::instance().loadPlates()) return 143;
    const auto *initialDefinitions=catalog.definitions().constData();
    QTemporaryDir temporary;
    if (!temporary.isValid()) return 130;
    const auto write = [&](const QString &stem, const QByteArray &data, bool png) {
        QFile file(temporary.filePath(stem+".info"));
        if (!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size()) return false;
        file.close();
        if (png) {
            QPixmap image(80,40); image.fill(Qt::blue);
            if (!image.save(temporary.filePath(stem+".png"))) return false;
        }
        return true;
    };
    // A previously unknown filename, tab, shape, label, and scale require no C++ class.
    if (!write("CustomPlate","Scenery\n19,7\nCustom display name\n0.625\n",true)
        || !write("NoPreviewScale","Plates\n8,16\nFull size\n0.0\n",true)
        || !write("MissingPng","Plates\n16,32\nMissing PNG\n0.75\n",false)
        || !write("BadSize","Plates\n0,32\nBad size\n0.75\n",true)
        || !write("BadScale","Plates\n16,32\nBad scale\n-1\n",true)
        || !write("BadLines","Plates\n16,32\n",true)
        || !write("EmptyTab","\n16,32\nNo tab\n0.75\n",true)
        || !write("EmptyLabel","Plates\n16,32\n\n0.75\n",true)
        || !write("NonFinite","Plates\n16,32\nBad scale\nnan\n",true)) return 131;
    QPixmap orphan(20,20); orphan.fill(Qt::red);
    if (!orphan.save(temporary.filePath("Orphan.png"))) return 132;
    const auto loaded = ElementCatalog::instance().loadElements(temporary.path());
    if (catalog.definitions().constData()!=initialDefinitions) return 144;
    if (loaded.size()!=2 || loaded[0].name!="CustomPlate" || loaded[0].tabName!="Scenery"
        || loaded[0].displayName!="Custom display name" || loaded[0].size!=QSize(19,7)
        || !near(loaded[0].previewScale,0.625)
        || loaded[0].resourcePath!=temporary.filePath("CustomPlate.png")
        || !near(loaded[1].previewScale,0.0)) return 133;

    LayoutElementsView palette;
    palette.addLayoutElements();
    palette.addLayoutElements(); // Repeated initialization must not duplicate entries.
    auto *tabs=palette.findChild<QTabWidget *>("layoutElementsTabs");
    if (!tabs) return 134;
    int plateCount=0;
    for (const auto &definition : ElementCatalog::instance().definitions()) {
        if (definition.tabName.isEmpty()) continue;
        if (definition.name!=QFileInfo(definition.resourcePath).completeBaseName()) return 142;
        if (definition.resourcePath.startsWith(":")) return 139;
        if (ElementCatalog::instance().byResource(":/images/"+QFileInfo(definition.resourcePath).fileName())) return 140;
        if (ElementCatalog::instance().byResource(definition.resourcePath)!=&definition) return 141;
        if (catalog.byName(definition.displayName)!=&definition) return 145;
        ++plateCount;
        QWidget *page=nullptr;
        for (int i=0;i<tabs->count();++i)
            if (tabs->tabText(i)==definition.tabName) page=tabs->widget(i);
        if (!page) return 135;
        LayoutElement *match=nullptr;
        for (auto *widget : page->findChildren<QWidget *>("layoutElementWidget")) {
            for (auto *label : widget->findChildren<QLabel *>()) {
                if (label->text()==definition.displayName) match=dynamic_cast<LayoutElement *>(widget);
            }
        }
        if (!match || match->plateSize()!=definition.size) return 136;
        QPixmap expected(definition.resourcePath);
        if (expected.width()>MAX_WIDTH || expected.height()>MAX_HEIGHT)
            expected=expected.scaled(QSize(MAX_WIDTH,MAX_HEIGHT),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        if (definition.previewScale>0)
            expected=expected.scaled(expected.size()*definition.previewScale,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        bool previewFound=false;
        for (auto *label : match->findChildren<QLabel *>())
            if (!label->pixmap().isNull() && label->pixmap().toImage()==expected.toImage()) previewFound=true;
        if (!previewFound) return 137;
    }
    if (plateCount==0 || ElementCatalog::instance().byName("16x32") || ElementCatalog::instance().byName("32x32")) return 138;
    qInfo()<<"PASS: .info pairing, custom metadata, invalid-file skipping, tab placement, stud sizes, labels, preview scaling, and initialization";
    return 0;
}
