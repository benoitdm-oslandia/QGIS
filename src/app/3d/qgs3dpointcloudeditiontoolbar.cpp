/***************************************************************************
    qgs3dpointcloudeditiontoolbar.cpp
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

#include "qgs3dpointcloudeditiontoolbar.h"
//#include "moc_qgs3dpointcloudeditiontoolbar.cpp"

#include <QAction>
#include <QShortcut>
#include <QWidget>
#include <QActionGroup>
#include <QComboBox>
#include <QMenu>
#include <QPointer>
#include <QLabel>

#include "qgsapplication.h"
#include "qgisapp.h"
#include "qgsdoublespinbox.h"
#include "qgs3dmapcanvas.h"
#include "qgs3dmaptoolpointcloudchangeattributepaintbrush.h"
#include "qgs3dmaptoolpointcloudchangeattributepolygon.h"
#include "qgspointcloudquerybuilder.h"
#include "qgspointclouddataprovider.h"
#include "qgspointcloudlayer3drenderer.h"

Qgs3DPointCloudEditionToolBar::Qgs3DPointCloudEditionToolBar( Qgs3DMapCanvasWidget *parent )
  : Qgs3DEditionToolBar( QStringLiteral( "Point cloud edition" ), parent )
{
  mEditingToolsMenu = new QMenu( this );

  mEditingToolsAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionSelectPolygon.svg" ) ), tr( "Select Editing Tool" ), this );
  mEditingToolsAction->setMenu( mEditingToolsMenu );
  /*mEditingToolBar->*/ addAction( mEditingToolsAction );

  QToolButton *editingToolsButton = qobject_cast<QToolButton *>( /*mEditingToolBar->*/ widgetForAction( mEditingToolsAction ) );
  editingToolsButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );
  QAction *actionPointCloudChangeAttributeTool = mEditingToolsMenu->addAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mActionSelectPolygon.svg" ) ) ), tr( "Select by Polygon" ), this, &Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByPolygon );
  QAction *actionPaintbrush = mEditingToolsMenu->addAction( QIcon( QgsApplication::iconPath( QStringLiteral( "propertyicons/rendering.svg" ) ) ), tr( "Select by Paintbrush" ), this, &Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByPaintbrush );
  QAction *actionAboveLineTool = mEditingToolsMenu->addAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mActionSelectAboveLine.svg" ) ) ), tr( "Select Above Line" ), this, &Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByAboveLine );
  QAction *actionBelowLineTool = mEditingToolsMenu->addAction( QIcon( QgsApplication::iconPath( QStringLiteral( "mActionSelectBelowLine.svg" ) ) ), tr( "Select Below Line" ), this, &Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByBelowLine );

  mGroupActions << actionPointCloudChangeAttributeTool << actionPaintbrush << actionAboveLineTool << actionBelowLineTool;

  QAction *actionPointFilter = addAction( QIcon( QgsApplication::iconPath( "mIconExpressionFilter.svg" ) ), tr( "Filter Points" ), this, &Qgs3DPointCloudEditionToolBar::changePointCloudAttributePointFilter );
  actionPointFilter->setCheckable( true );
  const QString tooltip = QStringLiteral( "%1\n\n%2\n%3" ).arg( tr( "Filter Points" ), tr( "Set an expression to filter points that should be edited." ), tr( "Points that do not satisfy the expression will not be modified." ) );
  actionPointFilter->setToolTip( tooltip );

  addWidget( new QLabel( tr( "Attribute" ) ) );
  mCboChangeAttribute = new QComboBox();
  addWidget( mCboChangeAttribute );
  mSpinChangeAttributeValue = new QgsDoubleSpinBox();
  mSpinChangeAttributeValue->setShowClearButton( false );
  addWidget( new QLabel( tr( "Value" ) ) );
  mSpinChangeAttributeValueAction = addWidget( mSpinChangeAttributeValue );
  mSpinChangeAttributeValueAction->setVisible( false );
  mCboChangeAttributeValue = new QComboBox();
  mCboChangeAttributeValue->setEditable( true );
  mClassValidator = new ClassValidator( this );
  mCboChangeAttributeValueAction = addWidget( mCboChangeAttributeValue );

  mMapToolChangeAttribute = new Qgs3DMapToolPointCloudChangeAttribute( mParentWidget->mapCanvas3D() );

  connect( mCboChangeAttribute, qOverload<int>( &QComboBox::currentIndexChanged ), this, [this]( int ) { onPointCloudChangeAttributeSettingsChanged(); } );
  connect( mCboChangeAttributeValue, qOverload<const QString &>( &QComboBox::currentTextChanged ), this, [this]( const QString &text ) {
    double newValue = 0;
    if ( mCboChangeAttributeValue->isEditable() )
    {
      const QStringList split = text.split( ' ' );
      if ( !split.isEmpty() )
      {
        newValue = split.constFirst().toDouble();
      }
    }
    else
    {
      newValue = mCboChangeAttributeValue->currentData().toDouble();
    }
    mMapToolChangeAttribute->setNewValue( newValue );
  } );
  connect( mSpinChangeAttributeValue, qOverload<double>( &QgsDoubleSpinBox::valueChanged ), this, [this]( double ) { mMapToolChangeAttribute->setNewValue( mSpinChangeAttributeValue->value() ); } );
}

void Qgs3DPointCloudEditionToolBar::deactivate()
{
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( false );

  mEditingToolsAction->setEnabled( false );
  qDebug() << __FUNCTION__ << __LINE__ << "visible:" << isVisible();
  setEnabled( false );
}

void Qgs3DPointCloudEditionToolBar::activate( QgsMapLayer *layer )
{
  QgsPointCloudLayer *pcLayer = qobject_cast<QgsPointCloudLayer *>( layer );
  const QVector<QgsPointCloudAttribute> attributes = pcLayer->attributes().attributes();
  const QString previousAttribute = mCboChangeAttribute->currentText();
  whileBlocking( mCboChangeAttribute )->clear();
  for ( const QgsPointCloudAttribute &attribute : attributes )
  {
    if ( attribute.name() == QLatin1String( "X" ) || attribute.name() == QLatin1String( "Y" ) || attribute.name() == QLatin1String( "Z" ) )
      continue;

    whileBlocking( mCboChangeAttribute )->addItem( attribute.name() );
  }

  int index = mCboChangeAttribute->findText( previousAttribute );
  if ( index < 0 )
    index = mCboChangeAttribute->findText( QStringLiteral( "Classification" ) );
  mCboChangeAttribute->setCurrentIndex( std::max( index, 0 ) );

  // setEnabled( pcLayer->isEditable() );
  mEditingToolsAction->setEnabled( pcLayer->isEditable() );
  // Reparse the class values when the renderer changes - renderer3DChanged() is not fired when only the renderer symbol is changed
  connect( pcLayer, &QgsMapLayer::request3DUpdate, this, &Qgs3DPointCloudEditionToolBar::onPointCloudChangeAttributeSettingsChanged );

  setEnabled( true );
  for ( auto action : findChildren<QAction *>() )
    action->setVisible( true );
  qDebug() << __FUNCTION__ << __LINE__ << "visible:" << isVisible();
}

void Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByPaintbrush()
{
  const QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mParentWidget->mapCanvas3D()->requestActivate();
  mMapToolChangeAttribute->deleteLater();
  mMapToolChangeAttribute = new Qgs3DMapToolPointCloudChangeAttributePaintbrush( mParentWidget->mapCanvas3D() );
  onPointCloudChangeAttributeSettingsChanged();
  mParentWidget->mapCanvas3D()->setMapTool( mMapToolChangeAttribute );
  mEditingToolsAction->setIcon( action->icon() );
}

void Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByPolygon()
{
  const QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mMapToolChangeAttribute->deleteLater();
  mMapToolChangeAttribute = new Qgs3DMapToolPointCloudChangeAttributePolygon( mParentWidget->mapCanvas3D(), Qgs3DMapToolPointCloudChangeAttributePolygon::Polygon );
  onPointCloudChangeAttributeSettingsChanged();
  mParentWidget->mapCanvas3D()->setMapTool( mMapToolChangeAttribute );
  mEditingToolsAction->setIcon( action->icon() );
}

void Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByAboveLine()
{
  const QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mMapToolChangeAttribute->deleteLater();
  mMapToolChangeAttribute = new Qgs3DMapToolPointCloudChangeAttributePolygon( mParentWidget->mapCanvas3D(), Qgs3DMapToolPointCloudChangeAttributePolygon::AboveLine );
  onPointCloudChangeAttributeSettingsChanged();
  mParentWidget->mapCanvas3D()->setMapTool( mMapToolChangeAttribute );
  mEditingToolsAction->setIcon( action->icon() );
}

void Qgs3DPointCloudEditionToolBar::changePointCloudAttributeByBelowLine()
{
  const QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mMapToolChangeAttribute->deleteLater();
  mMapToolChangeAttribute = new Qgs3DMapToolPointCloudChangeAttributePolygon( mParentWidget->mapCanvas3D(), Qgs3DMapToolPointCloudChangeAttributePolygon::BelowLine );
  onPointCloudChangeAttributeSettingsChanged();
  mParentWidget->mapCanvas3D()->setMapTool( mMapToolChangeAttribute );
  mEditingToolsAction->setIcon( action->icon() );
}

void Qgs3DPointCloudEditionToolBar::changePointCloudAttributePointFilter()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  QgsPointCloudLayer *layer = qobject_cast<QgsPointCloudLayer *>( QgisApp::instance()->activeLayer() );
  if ( !layer )
    return;

  QgsPointCloudQueryBuilder qb( layer, this );
  qb.setSubsetString( mChangeAttributePointFilter );
  if ( qb.exec() )
  {
    mChangeAttributePointFilter = qb.subsetString();
    mMapToolChangeAttribute->setPointFilter( mChangeAttributePointFilter );
  }
  action->setChecked( !mChangeAttributePointFilter.isEmpty() );
  QString tooltip = QStringLiteral( "%1\n\n%2\n%3" ).arg( tr( "Filter Points" ), tr( "Set an expression to filter points that should be edited." ), tr( "Points that do not satisfy the expression will not be modified." ) );
  if ( !mChangeAttributePointFilter.isEmpty() )
    tooltip.append( QStringLiteral( "\n%1\n%2" ).arg( tr( "Current filter expression: " ), mChangeAttributePointFilter ) );
  action->setToolTip( tooltip );
}

void Qgs3DPointCloudEditionToolBar::onPointCloudChangeAttributeSettingsChanged()
{
  const QString attributeName = mCboChangeAttribute->currentText();

  mSpinChangeAttributeValue->setSuffix( QString() );
  bool useComboBox = false;

  if ( attributeName == QLatin1String( "Intensity" ) || attributeName == QLatin1String( "PointSourceId" ) || attributeName == QLatin1String( "Red" ) || attributeName == QLatin1String( "Green" ) || attributeName == QLatin1String( "Blue" ) || attributeName == QLatin1String( "Infrared" ) )
  {
    mSpinChangeAttributeValue->setMinimum( 0 );
    mSpinChangeAttributeValue->setMaximum( 65535 );
    mSpinChangeAttributeValue->setDecimals( 0 );
  }
  else if ( attributeName == QLatin1String( "ReturnNumber" ) || attributeName == QLatin1String( "NumberOfReturns" ) )
  {
    mSpinChangeAttributeValue->setMinimum( 0 );
    mSpinChangeAttributeValue->setMaximum( 15 );
    mSpinChangeAttributeValue->setDecimals( 0 );
  }
  else if ( attributeName == QLatin1String( "Synthetic" ) || attributeName == QLatin1String( "KeyPoint" ) || attributeName == QLatin1String( "Withheld" ) || attributeName == QLatin1String( "Overlap" ) || attributeName == QLatin1String( "ScanDirectionFlag" ) || attributeName == QLatin1String( "EdgeOfFlightLine" ) )
  {
    useComboBox = true;
    const int oldIndex = mCboChangeAttributeValue->currentIndex();
    QgsSignalBlocker< QComboBox > blocker( mCboChangeAttributeValue );
    mCboChangeAttributeValue->clear();
    mCboChangeAttributeValue->addItem( tr( "False" ), 0 );
    mCboChangeAttributeValue->addItem( tr( "True" ), 1 );
    mCboChangeAttributeValue->setEditable( false );
    mCboChangeAttributeValue->setCurrentIndex( std::min( oldIndex, 1 ) );
  }
  else if ( attributeName == QLatin1String( "ScannerChannel" ) )
  {
    mSpinChangeAttributeValue->setMinimum( 0 );
    mSpinChangeAttributeValue->setMaximum( 3 );
    mSpinChangeAttributeValue->setDecimals( 0 );
  }
  else if ( attributeName == QLatin1String( "Classification" ) )
  {
    useComboBox = true;
    const QStringList split = mCboChangeAttributeValue->currentText().split( ' ' );
    const int oldValue = split.isEmpty() ? 0 : split.constFirst().toInt();

    whileBlocking( mCboChangeAttributeValue )->clear();
    // We will fill the combobox with all available classes from the Classification renderer (may have changed names) and the layer statistics
    // Users will be able to manually type in any other class number too.
    QMap<int, QString> lasCodes = QgsPointCloudDataProvider::translatedLasClassificationCodes();
    QMap<int, QString> classes;

    QgsPointCloudLayer *layer = qobject_cast<QgsPointCloudLayer *>( QgisApp::instance()->activeLayer() );
    if ( layer )
    {
      QgsAbstract3DRenderer *r = layer->renderer3D();
      // if there's a classification renderer, let's use the classes labels
      if ( QgsPointCloudLayer3DRenderer *cr = dynamic_cast<QgsPointCloudLayer3DRenderer *>( r ) )
      {
        const QgsPointCloud3DSymbol *s = cr->symbol();
        if ( const QgsClassificationPointCloud3DSymbol *cs = dynamic_cast<const QgsClassificationPointCloud3DSymbol *>( s ) )
        {
          if ( cs->attribute() == QLatin1String( "Classification" ) )
          {
            for ( const QgsPointCloudCategory &c : cs->categoriesList() )
            {
              classes[c.value()] = c.label();
            }
          }
        }
      }

      // then add missing classes from the layer stats too
      const QMap<int, int> statisticsClasses = layer->statistics().availableClasses( QStringLiteral( "Classification" ) );
      for ( auto it = statisticsClasses.constBegin(); it != statisticsClasses.constEnd(); ++it )
      {
        if ( !classes.contains( it.key() ) )
          classes[it.key()] = lasCodes[it.key()];
      }
      for ( auto it = classes.constBegin(); it != classes.constEnd(); ++it )
      {
        // populate the combobox
        whileBlocking( mCboChangeAttributeValue )->addItem( QStringLiteral( "%1 (%2)" ).arg( it.key() ).arg( it.value() ), it.key() );
        // and also update the labels in the full list of classes, which will be used in the editable combobox validator.
        lasCodes[it.key()] = it.value();
      }
    }
    // new values (manually edited) will be added after a separator
    mCboChangeAttributeValue->insertSeparator( mCboChangeAttributeValue->count() );
    mClassValidator->setClasses( lasCodes );
    mCboChangeAttributeValue->setEditable( true );
    mCboChangeAttributeValue->setValidator( mClassValidator );
    mCboChangeAttributeValue->setCompleter( nullptr );

    // Try to reselect last selected value
    if ( classes.contains( oldValue ) )
    {
      for ( int i = 0; i < mCboChangeAttributeValue->count(); ++i )
      {
        if ( mCboChangeAttributeValue->itemText( i ).startsWith( QStringLiteral( "%1 " ).arg( oldValue ) ) )
        {
          mCboChangeAttributeValue->setCurrentIndex( i );
          break;
        }
      }
    }
    else
    {
      whileBlocking( mCboChangeAttributeValue )->addItem( QStringLiteral( "%1 ()" ).arg( oldValue ), oldValue );
      mCboChangeAttributeValue->setCurrentIndex( mCboChangeAttributeValue->count() - 1 );
    }
  }
  else if ( attributeName == QLatin1String( "UserData" ) )
  {
    mSpinChangeAttributeValue->setMinimum( 0 );
    mSpinChangeAttributeValue->setMaximum( 255 );
    mSpinChangeAttributeValue->setDecimals( 0 );
  }
  else if ( attributeName == QLatin1String( "ScanAngleRank" ) )
  {
    mSpinChangeAttributeValue->setMinimum( -180 );
    mSpinChangeAttributeValue->setMaximum( 180 );
    mSpinChangeAttributeValue->setDecimals( 3 );
    mSpinChangeAttributeValue->setSuffix( QStringLiteral( " %1" ).arg( tr( "degrees" ) ) );
  }
  else if ( attributeName == QLatin1String( "GpsTime" ) )
  {
    mSpinChangeAttributeValue->setMinimum( 0 );
    mSpinChangeAttributeValue->setMaximum( std::numeric_limits<double>::max() );
    mSpinChangeAttributeValue->setDecimals( 42 );
  }

  mMapToolChangeAttribute->setAttribute( attributeName );
  double newValue = 0;
  if ( useComboBox && mCboChangeAttributeValue->isEditable() )
  {
    // read class integer
    const QStringList split = mCboChangeAttributeValue->currentText().split( ' ' );
    if ( !split.isEmpty() )
      newValue = split.constFirst().toDouble();
  }
  else if ( useComboBox )
  {
    // read true/false combo box
    newValue = mCboChangeAttributeValue->currentData().toDouble();
  }
  else
  {
    // read the spinbox value
    newValue = mSpinChangeAttributeValue->value();
  }
  mMapToolChangeAttribute->setNewValue( newValue );

  mCboChangeAttributeValueAction->setVisible( useComboBox );
  mSpinChangeAttributeValueAction->setVisible( !useComboBox );

  mMapToolChangeAttribute->setPointFilter( mChangeAttributePointFilter );
}

QList<QAction *> Qgs3DPointCloudEditionToolBar::groupActions() const
{
  return mGroupActions;
}
