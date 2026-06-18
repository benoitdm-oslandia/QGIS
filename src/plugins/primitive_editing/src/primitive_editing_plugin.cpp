/***************************************************************************
    primitive_editing_plugin.cpp

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

#include "primitive_editing_plugin.h"

#include "qgisinterface.h"
#include "qgs3dmapcanvas.h"
#include "qgs3dmapcanvaswidgetinterface.h"
#include "qgs3dprimitiveeditingtoolbar.h"

#include <QAction>
#include <QString>
#include <QThread>

#include "moc_primitive_editing_plugin.cpp"

using namespace Qt::StringLiterals;

static const QString sName = QObject::tr( "3DPrimitiveEditing" );
static const QString sDescription = QObject::tr( "Allow 3D primitive editing" );
static const QString sCategory = QObject::tr( "3D" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PluginType sPluginType = QgisPlugin::UI;
static const QString sPluginIcon = u":/plugin/mIcon3DAddTorus.svg"_s;

QgsPrimitiveEditingPlugin::QgsPrimitiveEditingPlugin( QgisInterface *qgisInterface )
  : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
  , mQGisIface( qgisInterface )
{}

QgsPrimitiveEditingPlugin::~QgsPrimitiveEditingPlugin()
{
  //delete mPrimitiveEditing;
}

void QgsPrimitiveEditingPlugin::initGui()
{
  // for existing 3D canvas
  connect( mQGisIface, &QgisInterface::projectRead, this, &QgsPrimitiveEditingPlugin::addToolbarToNewProject );

  // for future 3D canvas
  connect( mQGisIface->actionNew3DMapCanvas(), &QAction::triggered, this, &QgsPrimitiveEditingPlugin::addToolbarToNew3DCanvas );
}

void QgsPrimitiveEditingPlugin::addToolbarTo3DCanvas( Qgs3DMapCanvasWidgetInterface *mapView )
{
  if ( mapView )
  {
    for ( Qgs3DEditingToolBar *tb : mapView->editingToolBars() )
    {
      if ( dynamic_cast<Qgs3DPrimitiveEditingToolBar *>( tb ) )
        break;
    }
    mapView->addEditingToolBar( new Qgs3DPrimitiveEditingToolBar( mapView ) );
  }
}

void QgsPrimitiveEditingPlugin::addToolbarToNewProject()
{
  for ( Qgs3DMapCanvas *canvas : mQGisIface->mapCanvases3D() )
  {
    addToolbarTo3DCanvas( canvas->canvasWidgetInterface() );
  }
}

void QgsPrimitiveEditingPlugin::addToolbarToNew3DCanvas()
{
  // wait for the new 3D canvas to be created
  QThread::yieldCurrentThread();
  QThread::msleep( 100 );
  QThread::yieldCurrentThread();

  addToolbarTo3DCanvas( mQGisIface->mapCanvases3D().last()->canvasWidgetInterface() );
}

void QgsPrimitiveEditingPlugin::help()
{}

void QgsPrimitiveEditingPlugin::unload()
{
  connect( mQGisIface->actionNew3DMapCanvas(), &QAction::triggered, this, &QgsPrimitiveEditingPlugin::addToolbarToNew3DCanvas );
}

/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin *classFactory( QgisInterface *qgisInterfacePointer )
{
  return new QgsPrimitiveEditingPlugin( qgisInterfacePointer );
}

// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN const QString *name()
{
  return &sName;
}

// Return the description
QGISEXTERN const QString *description()
{
  return &sDescription;
}

// Return the category
QGISEXTERN const QString *category()
{
  return &sCategory;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN const QString *version()
{
  return &sPluginVersion;
}

QGISEXTERN const QString *icon()
{
  return &sPluginIcon;
}

// Delete ourself
QGISEXTERN void unload( QgisPlugin *pluginPointer )
{
  delete pluginPointer;
}
