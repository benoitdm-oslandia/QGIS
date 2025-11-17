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

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

#include "qgsapplication.h"
#include "qgs3dmapcanvas.h"
#include "qgs3dmaptoolpointcloudchangeattributepolygon.h"

Qgs3DPrimitiveEditionToolBar::Qgs3DPrimitiveEditionToolBar( Qgs3DMapCanvasWidget *parent )
  : Qgs3DEditionToolBar( QStringLiteral( "Primitive edition" ), parent )
{
  addWidget( new QLabel( tr( "PRIMITIVE" ) ) );

  mEditingToolsMenu = new QMenu( this );

  mEditingToolsAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionAddBasicShape.svg" ) ), tr( "Create new primitive" ), this );
  mEditingToolsAction->setMenu( mEditingToolsMenu );
  addAction( mEditingToolsAction );

  QToolButton *editingToolsButton = qobject_cast<QToolButton *>( widgetForAction( mEditingToolsAction ) );
  editingToolsButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );
  /* QAction *actionPointCloudChangeAttributeTool =*/mEditingToolsMenu->addAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mIconEsriI3s.svg" ) ) ), tr( "Create a cube" ), this, &Qgs3DPrimitiveEditionToolBar::changePointCloudAttributeByPolygon );
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
  return {};
}

void Qgs3DPrimitiveEditionToolBar::changePointCloudAttributeByPolygon()
{
  const QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mMapToolChangeAttribute->deleteLater();
  mMapToolChangeAttribute = new Qgs3DMapToolPointCloudChangeAttributePolygon( mParentWidget->mapCanvas3D(), Qgs3DMapToolPointCloudChangeAttributePolygon::Polygon );
  //onPointCloudChangeAttributeSettingsChanged();
  mParentWidget->mapCanvas3D()->setMapTool( mMapToolChangeAttribute );
  mEditingToolsAction->setIcon( action->icon() );
}
