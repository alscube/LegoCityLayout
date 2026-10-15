#include "ProjectView.h"
#include "LoadedProjects.h"
#include "elements/CityLayoutElement.h"
#include <QApplication>
#include <QAbstractButton>
#include <QPushButton>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid()) return 1;
    QCoreApplication::setOrganizationName("LegoCityLayoutTests");
    QCoreApplication::setApplicationName("MissingElements");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temporary.path());
    ProjectView editor;
    editor.startProject("Existing project");
    auto &projects=LoadedProjects::instance();
    const int originalIndex=projects.currentProjectIndex(), originalCount=projects.projectCount();
    auto *original=projects.currentProject();
    const QString path=temporary.filePath("missing.LegoLayout");
    const auto placement=[](const QString &name, qreal x, qreal rotation=0) {
        return QJsonObject{{"name",name},{"x",x},{"y",200.5},{"rotationDegrees",rotation}};
    };
    const QJsonObject straight=placement("Straight Track",100.25,22.5);
    const QJsonObject curve=placement("Curved Track (22.5°)",300.75,45);
    const QJsonArray mixed{placement("Removed plate",10),straight,placement("Removed plate",20),
                          curve,placement("Removed building",30)};
    const QJsonArray outline{QJsonObject{{"x",0},{"y",0}},QJsonObject{{"x",500},{"y",0}},
                            QJsonObject{{"x",500},{"y",300}},QJsonObject{{"x",0},{"y",300}}};
    QJsonObject root{{"format","LegoCityLayout.TableDefinition"},{"version",2},
        {"layoutName","Recovered project"},{"openSides",QJsonArray()},
        {"surfaces",QJsonArray{QJsonObject{{"name","Desk"},{"outline",outline}}}}};
    const auto write=[&](const QJsonArray &elements) {
        root.insert("cityLayout",QJsonObject{{"elements",elements},{"zoomFactor",1.875},
            {"gridOriginX",11.25},{"gridOriginY",22.5}});
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return QByteArray();
        const auto data=QJsonDocument(root).toJson();
        if (file.write(data)!=data.size()) return QByteArray();
        return data;
    };
    const auto bytes=[&] {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    struct Outcome {
        bool opened=false, defaultNo=true;
        int missingPrompts=0, errors=0;
        QString text;
    };
    const auto open=[&](QMessageBox::StandardButton reply, bool last, bool dismiss=false) {
        Outcome result;
        const int before=projects.projectCount();
        QTimer responder;
        QObject::connect(&responder,&QTimer::timeout,[&] {
            auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (!box) return;
            // macOS may ignore message-box window titles; identify the dialog by its buttons.
            if (box->standardButtons()==(QMessageBox::Yes | QMessageBox::No)) {
                ++result.missingPrompts;
                result.text=box->text();
                result.defaultNo &= box->standardButton(box->defaultButton())==QMessageBox::No;
                if (dismiss) box->reject();
                else if (auto *button=box->button(reply)) button->click();
                else box->reject();
            } else {
                ++result.errors;
                result.text=box->text();
                qInfo()<<"Loader error:"<<box->text();
                box->done(QMessageBox::Ok);
            }
        });
        responder.start(10);
        if (last) {
            QSettings().setValue("layout/lastPath",path);
            editor.openLastLayout();
            result.opened=projects.projectCount()>before;
        } else result.opened=editor.openLayout(path);
        return result;
    };
    const QByteArray mixedBytes=write(mixed);
    if (mixedBytes.isEmpty()) return 2;
    const auto cancelled=open(QMessageBox::No,true);
    if (cancelled.opened || cancelled.missingPrompts!=1 || cancelled.errors || !cancelled.defaultNo
        || projects.projectCount()!=originalCount || projects.currentProjectIndex()!=originalIndex
        || projects.currentProject()!=original || bytes()!=mixedBytes) {
        qCritical()<<"Cancel check"<<cancelled.opened<<cancelled.missingPrompts<<cancelled.errors<<cancelled.defaultNo
            <<projects.projectCount()<<originalCount<<projects.currentProjectIndex()<<originalIndex
            <<(projects.currentProject()==original)<<(bytes()==mixedBytes);
        return 3;
    }
    if (!cancelled.text.contains("3 saved") || cancelled.text.count("Removed plate")!=1
        || cancelled.text.count("Removed building")!=1 || !cancelled.text.contains("omitted")) return 4;
    const auto dismissed=open(QMessageBox::No,true,true);
    if (dismissed.opened || dismissed.missingPrompts!=1 || dismissed.errors
        || projects.projectCount()!=originalCount || projects.currentProject()!=original) return 5;
    const auto accepted=open(QMessageBox::Yes,true);
    if (!accepted.opened || accepted.missingPrompts!=1 || accepted.errors
        || projects.projectCount()!=originalCount+1 || projects.cityLayouts().size()!=2
        || projects.title()!="Recovered project" || projects.tableDefinition().surfaces().size()!=1
        || projects.currentProject()->gridOrigin!=QPointF(11.25,22.5) || bytes()!=mixedBytes) return 6;
    const auto *first=projects.cityLayouts()[0];
    const auto *second=projects.cityLayouts()[1];
    if (first->name()!="Straight Track" || second->name()!="Curved Track (22.5°)"
        || first->layoutPosition()!=QPointF(100.25,200.5) || first->rotationDegrees()!=22.5
        || second->layoutPosition()!=QPointF(300.75,200.5) || second->rotationDegrees()!=45) return 7;
    if (write(QJsonArray{straight,curve}).isEmpty()) return 8;
    const auto complete=open(QMessageBox::No,false);
    if (!complete.opened || complete.missingPrompts || complete.errors || projects.cityLayouts().size()!=2) return 9;
    if (write(QJsonArray{placement("Removed plate",10),placement("Removed building",20)}).isEmpty()) return 10;
    const auto empty=open(QMessageBox::Yes,false);
    if (!empty.opened || empty.missingPrompts!=1 || empty.errors || !projects.cityLayouts().isEmpty()
        || projects.tableDefinition().surfaces().size()!=1) return 11;
    const int validCount=projects.projectCount();
    auto invalid=straight;
    invalid.insert("rotationDegrees",-1);
    if (write(QJsonArray{placement("Removed plate",10),invalid}).isEmpty()) return 12;
    const auto corrupt=open(QMessageBox::Yes,false);
    if (corrupt.opened || corrupt.missingPrompts || corrupt.errors!=1 || projects.projectCount()!=validCount) return 13;
    projects.clear();
    qInfo()<<"PASS: Open Last Layout Yes/No/close; one prompt with deduplicated missing types; known placements and table retained; original file untouched; valid and corrupt layouts unchanged";
    return 0;
}
