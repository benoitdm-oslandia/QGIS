/***************************************************************************
    Qgs3DPrimitiveEditingToolBar.cpp
    -------------------
    begin                : November 2025
    copyright            : (C) 2025 Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dprimitiveeditingtoolbar.h"

#include "qgs3dmapcanvas.h"
#include "qgs3dmapcanvaswidgetinterface.h"
#include "qgsvectorlayer.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QString>
#include <QToolButton>

using namespace Qt::StringLiterals;

Qgs3DPrimitiveEditingToolBar::Qgs3DPrimitiveEditingToolBar( Qgs3DMapCanvasWidgetInterface *parent )
  : Qgs3DEditingToolBar( u"Primitive editing"_s, dynamic_cast<QWidget *>( parent ) )
{
  addWidget( new QLabel( tr( "PRIMITIVE" ) ) );

  mCreatePrimitiveAction = new QAction( QIcon( u":/plugin/mActionAddBasicShape.svg"_s ), tr( "Create new primitive" ), this );

  QMenu *createPrimitiveMenu = new QMenu( this );
  mCreatePrimitiveAction->setMenu( createPrimitiveMenu );

  addAction( mCreatePrimitiveAction );
  QToolButton *createPrimitiveButton = qobject_cast<QToolButton *>( widgetForAction( mCreatePrimitiveAction ) );
  createPrimitiveButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  mActions << createPrimitiveMenu->addAction( QIcon( u":/plugin/mIcon3DAddBox.svg"_s ), tr( "Create a box" ), this, &Qgs3DPrimitiveEditingToolBar::createBox );
  mActions << createPrimitiveMenu->addAction( QIcon( u":/plugin/mIcon3DAddSphere.svg"_s ), tr( "Create a sphere" ), this, &Qgs3DPrimitiveEditingToolBar::createSphere );
  mActions << createPrimitiveMenu->addAction( QIcon( u":/plugin/mIcon3DAddTorus.svg"_s ), tr( "Create a torus" ), this, &Qgs3DPrimitiveEditingToolBar::createTorus );
  mActions << createPrimitiveMenu->addAction( QIcon( u":/plugin/mIcon3DAddCylinder.svg"_s ), tr( "Create a cylinder" ), this, &Qgs3DPrimitiveEditingToolBar::createCylinder );
  mActions << createPrimitiveMenu->addAction( QIcon( u":/plugin/mIcon3DAddCone.svg"_s ), tr( "Create a cone" ), this, &Qgs3DPrimitiveEditingToolBar::createCone );
}

bool Qgs3DPrimitiveEditingToolBar::accept( QgsMapLayer *layer ) const
{
  if ( layer == nullptr || layer->type() != Qgis::LayerType::Vector )
    return false;
  const QgsVectorLayer *vl = dynamic_cast<QgsVectorLayer *>( layer );
  return vl != nullptr && QgsWkbTypes::flatType( vl->wkbType() ) == Qgis::WkbType::PolyhedralSurface;
}


void Qgs3DPrimitiveEditingToolBar::activate( QgsMapLayer *layer )
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( true );

  setEnabled( true );
  mActiveLayer = layer;
}

void Qgs3DPrimitiveEditingToolBar::deactivate()
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( false );

  setEnabled( false );
  // disable current map tool if defined
  if ( mCreatePrimitiveMapTool )
  {
    mCreatePrimitiveMapTool->deleteLater();
    mCreatePrimitiveMapTool = nullptr;
    dynamic_cast<Qgs3DMapCanvasWidgetInterface *>( mParentWidget )->mapCanvas3D()->setMapTool( mCreatePrimitiveMapTool );
  }
  // revert to default icon
  mCreatePrimitiveAction->setIcon( QIcon( u":/plugin/mActionAddBasicShape.svg"_s ) );
}

QList<QAction *> Qgs3DPrimitiveEditingToolBar::groupActions() const
{
  return mActions;
}

void Qgs3DPrimitiveEditingToolBar::createBox()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Box );
}

void Qgs3DPrimitiveEditingToolBar::createSphere()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Sphere );
}

void Qgs3DPrimitiveEditingToolBar::createTorus()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Torus );
}

void Qgs3DPrimitiveEditingToolBar::createCylinder()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Cylinder );
}

void Qgs3DPrimitiveEditingToolBar::createCone()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Cone );
}

void Qgs3DPrimitiveEditingToolBar::createPrimitive( const QAction *action, Qgs3DMapToolCreatePrimitive::PrimitiveType type )
{
  if ( !action )
    return;

  if ( mCreatePrimitiveMapTool != nullptr )
    mCreatePrimitiveMapTool->deleteLater();

  Qgs3DMapCanvasWidgetInterface *canvasWidget = dynamic_cast<Qgs3DMapCanvasWidgetInterface *>( mParentWidget );
  mCreatePrimitiveMapTool = new Qgs3DMapToolCreatePrimitive( canvasWidget->mapCanvas3D(), canvasWidget->lateralPanel(), mActiveLayer, type );
  canvasWidget->mapCanvas3D()->setMapTool( mCreatePrimitiveMapTool );
  mCreatePrimitiveAction->setIcon( action->icon() );
}
