/***************************************************************************
    qgs3dprimitiveeditiontoolbar.cpp
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

#include "qgs3dprimitiveeditiontoolbar.h"

#include "qgs3dmapcanvas.h"
#include "qgs3dmaptoolcreateprimitive.h"
#include "qgsapplication.h"
#include "qgsvectorlayer.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

Qgs3DPrimitiveEditionToolBar::Qgs3DPrimitiveEditionToolBar( Qgs3DMapCanvasWidget *parent )
  : Qgs3DEditionToolBar( u"Primitive edition"_s, parent )
{
  addWidget( new QLabel( tr( "PRIMITIVE" ) ) );

  mCreatePrimitiveAction = new QAction( QgsApplication::getThemeIcon( u"mActionAddBasicShape.svg"_s ), tr( "Create new primitive" ), this );

  QMenu *createPrimitiveMenu = new QMenu( this );
  mCreatePrimitiveAction->setMenu( createPrimitiveMenu );

  addAction( mCreatePrimitiveAction );
  QToolButton *createPrimitiveButton = qobject_cast<QToolButton *>( widgetForAction( mCreatePrimitiveAction ) );
  createPrimitiveButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  mActions << createPrimitiveMenu->addAction( QIcon( QgsApplication::iconPath( u"mIcon3DAddCube.svg"_s ) ), tr( "Create a cube" ), this, &Qgs3DPrimitiveEditionToolBar::createCube );
  mActions << createPrimitiveMenu->addAction( QIcon( QgsApplication::iconPath( u"mIcon3DAddSphere.svg"_s ) ), tr( "Create a sphere" ), this, &Qgs3DPrimitiveEditionToolBar::createSphere );
  mActions << createPrimitiveMenu->addAction( QIcon( QgsApplication::iconPath( u"mIcon3DAddTorus.svg"_s ) ), tr( "Create a torus" ), this, &Qgs3DPrimitiveEditionToolBar::createTorus );
  mActions << createPrimitiveMenu->addAction( QIcon( QgsApplication::iconPath( u"mIcon3DAddCylinder.svg"_s ) ), tr( "Create a cylinder" ), this, &Qgs3DPrimitiveEditionToolBar::createCylinder );
  mActions << createPrimitiveMenu->addAction( QIcon( QgsApplication::iconPath( u"mIcon3DAddCone.svg"_s ) ), tr( "Create a cone" ), this, &Qgs3DPrimitiveEditionToolBar::createCone );
}

bool Qgs3DPrimitiveEditionToolBar::accept( QgsMapLayer *layer ) const
{
  if ( layer == nullptr || layer->type() != Qgis::LayerType::Vector )
    return false;
  const QgsVectorLayer *vl = dynamic_cast<QgsVectorLayer *>( layer );
  return vl != nullptr && QgsWkbTypes::flatType( vl->wkbType() ) == Qgis::WkbType::PolyhedralSurface;
}


void Qgs3DPrimitiveEditionToolBar::activate( QgsMapLayer * /*layer*/ )
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( true );

  setEnabled( true );
}

void Qgs3DPrimitiveEditionToolBar::deactivate()
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( false );

  setEnabled( false );
}

QList<QAction *> Qgs3DPrimitiveEditionToolBar::groupActions() const
{
  return mActions;
}

void Qgs3DPrimitiveEditionToolBar::createCube()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Cube );
}

void Qgs3DPrimitiveEditionToolBar::createSphere()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Sphere );
}

void Qgs3DPrimitiveEditionToolBar::createTorus()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Torus );
}

void Qgs3DPrimitiveEditionToolBar::createCylinder()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Cylinder );
}

void Qgs3DPrimitiveEditionToolBar::createCone()
{
  createPrimitive( qobject_cast<QAction *>( sender() ), Qgs3DMapToolCreatePrimitive::Cone );
}

void Qgs3DPrimitiveEditionToolBar::createPrimitive( const QAction *action, Qgs3DMapToolCreatePrimitive::PrimitiveType type )
{
  if ( !action )
    return;

  if ( mCreatePrimitiveMapTool != nullptr )
    mCreatePrimitiveMapTool->deleteLater();

  mCreatePrimitiveMapTool = new Qgs3DMapToolCreatePrimitive( mParentWidget->mapCanvas3D(), type );
  mParentWidget->mapCanvas3D()->setMapTool( mCreatePrimitiveMapTool );
  mCreatePrimitiveAction->setIcon( action->icon() );
}
