/***************************************************************************
    qgs3dsnappingtoolbar.cpp
    -------------------
    begin                : December 2025
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

#include "qgs3dsnappingtoolbar.h"

#include <QMenu>
#include <QToolButton>
#include <QSizePolicy>

#include "qgsdoublespinbox.h"
#include "qgssettings.h"
#include "qgs3dsnappingmanager.h"
#include "qgsapplication.h"
#include "qgs3dmapsettings.h"
#include "qgsunittypes.h"

Qgs3DSnappingToolbar::Qgs3DSnappingToolbar( Qgs3DMapCanvasWidget *parent, Qgs3DSnappingManager *snapper, const QgsSettings &setting )
  : QToolBar( tr( "Snapping Toolbar" ), parent ), mSnapper( snapper )
{
  setMovable( true );
  setVisible( setting.value( QStringLiteral( "/3D/snappingToolbar/visibility" ), false, QgsSettings::Gui ).toBool() );
  connect( this, &QToolBar::visibilityChanged, this, [this]() {
    QgsSettings setting;
    setting.setValue( QStringLiteral( "/3D/snappingToolbar/visibility" ), isVisible(), QgsSettings::Gui );
  } );

  mSnappingToleranceSpinBox = new QgsDoubleSpinBox();
  mSnappingToleranceSpinBox->setDecimals( 2 );
  mSnappingToleranceSpinBox->setMinimum( 0.0 );
  mSnappingToleranceSpinBox->setMaximum( 9999.99 );
  mSnappingToleranceSpinBox->setToolTip( tr( "Snapping Tolerance" ) );
  mSnappingToleranceSpinBox->setValue( mSnapper->tolerance() );
  mSnappingToleranceSpinBox->setEnabled( true );

  connect( mSnappingToleranceSpinBox, qOverload<double>( &QgsDoubleSpinBox::valueChanged ), this, [this]() {
    mSnapper->setTolerance( mSnappingToleranceSpinBox->value() );
  } );

  mSnappingUnitLabel = new QLabel( this );

  mSnappingAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mIconSnapping.svg" ) ), tr( "Snapping" ), this );

  QMenu *snappingMenu = new QMenu( this );
  mSnappingAction->setMenu( snappingMenu );
  addAction( mSnappingAction );

  QToolButton *snappingButton = qobject_cast<QToolButton *>( widgetForAction( mSnappingAction ) );
  snappingButton->setToolTip( tr( "Snapping Type" ) );
  snappingButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  QAction *snappingVertexAction = new QAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mIconSnappingEndPoint.svg" ) ) ), tr( "Snap on vertex" ) );
  snappingVertexAction->setData( QVariant::fromValue( Qgs3DSnappingManager::SnappingMode::Vertex ) );
  snappingVertexAction->setCheckable( true );
  snappingVertexAction->setChecked( mSnapper->snappingMode() & Qgs3DSnappingManager::SnappingMode::Vertex );
  snappingMenu->addAction( snappingVertexAction );

  QAction *snappingMidEdgeAction = new QAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mIconSnappingMiddle.svg" ) ) ), tr( "Snap on mid edge" ) );
  snappingMidEdgeAction->setData( QVariant::fromValue( Qgs3DSnappingManager::SnappingMode::MiddleEdge ) );
  snappingMidEdgeAction->setCheckable( true );
  snappingMidEdgeAction->setChecked( mSnapper->snappingMode() & Qgs3DSnappingManager::SnappingMode::MiddleEdge );
  snappingMenu->addAction( snappingMidEdgeAction );

  QAction *snappingAlongEdgeAction = new QAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mIconSnappingSegment.svg" ) ) ), tr( "Snap on edge" ) );
  snappingAlongEdgeAction->setData( QVariant::fromValue( Qgs3DSnappingManager::SnappingMode::AlongEdge ) );
  snappingAlongEdgeAction->setCheckable( true );
  snappingAlongEdgeAction->setChecked( mSnapper->snappingMode() & Qgs3DSnappingManager::SnappingMode::AlongEdge );
  snappingMenu->addAction( snappingAlongEdgeAction );

  QAction *snappingCenterFaceAction = new QAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mIconSnappingCentroid.svg" ) ) ), tr( "Snap on face center" ) );
  snappingCenterFaceAction->setData( QVariant::fromValue( Qgs3DSnappingManager::SnappingMode::CenterFace ) );
  snappingCenterFaceAction->setCheckable( true );
  snappingCenterFaceAction->setChecked( mSnapper->snappingMode() & Qgs3DSnappingManager::SnappingMode::CenterFace );
  snappingMenu->addAction( snappingCenterFaceAction );

  mSnappingAction->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "mIcon3DSnappingDisabled.svg" ) ) );
  connect( snappingButton, &QToolButton::triggered, this, &Qgs3DSnappingToolbar::onSnappingButtonTriggered );

  addWidget( mSnappingToleranceSpinBox );
  QWidget *snappingSpacer = new QWidget( this );
  snappingSpacer->setFixedWidth( 6 );
  addWidget( snappingSpacer );
  addWidget( mSnappingUnitLabel );

  onSnappingButtonTriggered( nullptr );
}

void Qgs3DSnappingToolbar::onSnappingButtonTriggered( QAction *action )
{
  if ( action != nullptr )
  {
    unsigned int newSnappingMode = static_cast<int>( mSnapper->snappingMode() );
    const Qgs3DSnappingManager::SnappingMode actionFlag = static_cast<Qgs3DSnappingManager::SnappingMode>( action->data().toInt() );
    newSnappingMode ^= actionFlag;

    mSnapper->setSnappingMode( static_cast<Qgs3DSnappingManager::SnappingMode>( newSnappingMode ) );
  }

  if ( mSnapper->snappingMode() == Qgs3DSnappingManager::SnappingMode::Off )
  {
    mSnappingAction->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "mIcon3DSnappingDisabled.svg" ) ) );
    mSnappingToleranceSpinBox->setEnabled( false );
    mSnappingUnitLabel->setEnabled( false );
  }
  else
  {
    mSnappingAction->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "mIconSnapping.svg" ) ) );
    mSnappingToleranceSpinBox->setEnabled( true );
    mSnappingUnitLabel->setEnabled( true );
  }
}

void Qgs3DSnappingToolbar::onSettingUpdate( const Qgs3DMapSettings *map )
{
  mSnappingUnitLabel->setText( QgsUnitTypes::encodeUnit( map->crs().mapUnits() ) );
}
