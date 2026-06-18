/***************************************************************************
    primitive_editing.h

    Primitive Editing Plugin
    a QGIS plugin
     --------------------------------------
    Date                 : 08-Jul-2010
    Copyright            : (C) 2010 by Sourcepole
    Email                : info at sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS_PRIMITIVE_EDITING_PLUGIN_H
#define QGS_PRIMITIVE_EDITING_PLUGIN_H

#include "qgisplugin.h"

#include <QObject>

class QAction;
class QgisInterface;
class QgsPrimitiveEditingProgressDialog;
class Qgs3DMapCanvasWidgetInterface;

class QgsPrimitiveEditingPlugin : public QObject, public QgisPlugin
{
    Q_OBJECT

  public:
    explicit QgsPrimitiveEditingPlugin( QgisInterface *qgisInterface );
    ~QgsPrimitiveEditingPlugin() override;

  public slots:
    //! init the gui
    void initGui() override;
    //! unload the plugin
    void unload() override;

    void addToolbarToNew3DCanvas();

    void addToolbarToNewProject();

    //! show the help document
    void help();

  private:
    //! Pointer to the QGIS interface object
    QgisInterface *mQGisIface = nullptr;

    void addToolbarTo3DCanvas( Qgs3DMapCanvasWidgetInterface *mapView );

  private slots:
};

#endif // QGS_PRIMITIVE_EDITING_PLUGIN_H
