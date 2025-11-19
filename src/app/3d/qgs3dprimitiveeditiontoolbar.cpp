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
#include "qgs3dmaptoolcreatecube.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

#include "qgsapplication.h"
#include "qgs3dmapcanvas.h"

Qgs3DPrimitiveEditionToolBar::Qgs3DPrimitiveEditionToolBar( Qgs3DMapCanvasWidget *parent )
  : Qgs3DEditionToolBar( QStringLiteral( "Primitive edition" ), parent )
{
  addWidget( new QLabel( tr( "PRIMITIVE" ) ) );

  mCreatePrimitiveAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionAddBasicShape.svg" ) ), tr( "Create new primitive" ), this );

  mCreatePrimitiveMenu = new QMenu( this );
  mCreatePrimitiveAction->setMenu( mCreatePrimitiveMenu );

  addAction( mCreatePrimitiveAction );
  QToolButton *createPrimitiveButton = qobject_cast<QToolButton *>( widgetForAction( mCreatePrimitiveAction ) );
  createPrimitiveButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  /* QAction *actionPointCloudChangeAttributeTool =*/mCreatePrimitiveMenu->addAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mIconEsriI3s.svg" ) ) ), tr( "Create a cube" ), this, &Qgs3DPrimitiveEditionToolBar::createCube );
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

void Qgs3DPrimitiveEditionToolBar::createCube()
{
  const QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  if ( mCreateCubeMapTool != nullptr )
    mCreateCubeMapTool->deleteLater();
  mCreateCubeMapTool = new Qgs3DMapToolCreateCube( mParentWidget->mapCanvas3D() );
  //onPointCloudChangeAttributeSettingsChanged();
  mParentWidget->mapCanvas3D()->setMapTool( mCreateCubeMapTool );
  mCreatePrimitiveAction->setIcon( action->icon() );
}
