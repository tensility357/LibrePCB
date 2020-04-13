/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBREPCB_LENGTHEDITBASE_H
#define LIBREPCB_LENGTHEDITBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../units/length.h"
#include "../units/lengthunit.h"
#include "numbereditbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class LengthEditBase
 ******************************************************************************/

/**
 * @brief The LengthEditBase class
 */
class LengthEditBase : public QAbstractSpinBox {
  Q_OBJECT

public:
  // Constructors / Destructor
  LengthEditBase() = delete;
  explicit LengthEditBase(const Length& min, const Length& max,
                          const Length& value,
                          QWidget*      parent = nullptr) noexcept;
  LengthEditBase(const LengthEditBase& other) = delete;
  virtual ~LengthEditBase() noexcept;

  // Getters
  const LengthUnit& getUnit() const noexcept { return mUnit; }

  // Setters
  void setUnit(const LengthUnit& unit) noexcept;
  void setChangeUnitActionVisible(bool visible) noexcept;

  // Predefined Configurations
  void configureForCoordinate(const LengthUnit& unit) noexcept;
  void configureForSize(const LengthUnit& unit) noexcept;
  void configureForLineWidth(const LengthUnit& unit) noexcept;
  void configureForTextHeight() noexcept;
  void configureForTextStrokeWidth() noexcept;
  void configureForPinLength(const LengthUnit& unit) noexcept;
  void configureForDrillDiameter(const LengthUnit& unit) noexcept;
  void configureForClearance(const LengthUnit& unit) noexcept;

  /**
   * @brief Set the supported up/down step values
   *
   * The step with lowest priority (typically the smallest value) must be the
   * first element in the list, the step with highest priority (typically the
   * largest value) the last one.
   *
   * Example:
   * {0.1mm, 1.0mm} leads to the steps 0.0mm, 0.1mm, .. 0.9mm, 1.0mm, 2.0mm, ...
   *
   * @param steps   List of supported step values
   */
  void setSteps(const QVector<PositiveLength>& steps) noexcept;

  // Reimplemented Methods
  QSize minimumSizeHint() const override;
  QSize sizeHint() const override;

  // Operator Overloadings
  LengthEditBase& operator=(const LengthEditBase& rhs) = delete;

protected:  // Methods
  virtual QAbstractSpinBox::StepEnabled stepEnabled() const override;
  virtual void                          stepBy(int steps) override;
  void                                  setValueImpl(Length value) noexcept;
  void         updateValueFromText(QString text) noexcept;
  void         updateSingleStep() noexcept;
  void         updateText() noexcept;
  LengthUnit   extractUnitFromExpression(QString& expression) const noexcept;
  void         changeUnitActionTriggered() noexcept;
  QString      getValueStr(const LengthUnit& unit) const noexcept;
  virtual void valueChangedImpl() noexcept = 0;

protected:  // Data
  QAction*                mChangeUnitAction;
  LengthUnit              mUnit;
  Length                  mMinimum;
  Length                  mMaximum;
  Length                  mValue;
  QVector<PositiveLength> mSteps;
  Length                  mSingleStepUp;    ///< Zero means "no step available"
  Length                  mSingleStepDown;  ///< Zero means "no step available"
  QSize                   mAdditionalSize;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_LENGTHEDITBASE_H
