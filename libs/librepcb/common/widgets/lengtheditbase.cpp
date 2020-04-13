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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "lengtheditbase.h"

#include "../toolbox.h"
#include "../utils/mathparser.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LengthEditBase::LengthEditBase(const Length& min, const Length& max,
                               const Length& value, QWidget* parent) noexcept
  : QAbstractSpinBox(parent),
    mChangeUnitAction(lineEdit()->addAction(QIcon(":/img/actions/ruler.png"),
                                            QLineEdit::TrailingPosition)),
    mUnit(LengthUnit::millimeters()),
    mMinimum(min),
    mMaximum(max),
    mValue(value),
    mSteps(),
    mSingleStepUp(0),
    mSingleStepDown(0),
    // Additional size for the QAction inside the QLineEdit because
    // QAbstractSpinBox does not respect it.
    mAdditionalSize(30, 0) {
  Q_ASSERT((mValue >= mMinimum) && (mValue <= mMaximum));

  // Ugly hack to make sizeHint() and minimumSizeHint() working properly.
  // QAbstractSpinBox uses (among others) the special value text to calculate
  // the size hint, so let's set it to a dummy string which is long enough to
  // represent typical length values.
  setSpecialValueText("000.000 mils");

  // Setup QLineEdit.
  lineEdit()->setPlaceholderText(tr("Enter numeric expression"));
  lineEdit()->setMaxLength(50);
  updateText();

  // Apply a default config.
  configureForCoordinate(LengthUnit::millimeters());

  // editingFinished from the QLineEdit is not always emitted (e.g. when
  // leaving focus), therefore we need to use editingFinished from
  // QAbstractSpinBox.
  connect(this, &LengthEditBase::editingFinished, this,
          &LengthEditBase::updateText);
  connect(lineEdit(), &QLineEdit::textEdited, this,
          &LengthEditBase::updateValueFromText);
  connect(mChangeUnitAction, &QAction::triggered, this,
          &LengthEditBase::changeUnitActionTriggered);
}

LengthEditBase::~LengthEditBase() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LengthEditBase::setUnit(const LengthUnit& unit) noexcept {
  if (unit != mUnit) {
    mUnit = unit;
    updateText();
  }
}

void LengthEditBase::setChangeUnitActionVisible(bool visible) noexcept {
  mChangeUnitAction->setVisible(visible);
}

void LengthEditBase::setSteps(const QVector<PositiveLength>& steps) noexcept {
  mSteps = steps;
  updateSingleStep();
  update();  // step buttons might need to be repainted
}

/*******************************************************************************
 *  Predefined Configurations
 ******************************************************************************/

void LengthEditBase::configureForCoordinate(const LengthUnit& unit) noexcept {
  setSteps({
      PositiveLength(10000),    // 0.01mm
      PositiveLength(25400),    // 0.0254mm
      PositiveLength(100000),   // 0.1mm
      PositiveLength(254000),   // 0.254mm
      PositiveLength(1000000),  // 1mm
      PositiveLength(2540000),  // 2.54mm
  });
  setUnit(unit);
}

void LengthEditBase::configureForSize(const LengthUnit& unit) noexcept {
  setSteps({
      PositiveLength(10000),    // 0.01mm
      PositiveLength(25400),    // 0.0254mm
      PositiveLength(100000),   // 0.1mm
      PositiveLength(254000),   // 0.254mm
      PositiveLength(1000000),  // 1mm
      PositiveLength(2540000),  // 2.54mm
  });
  setUnit(unit);
}

void LengthEditBase::configureForLineWidth(const LengthUnit& unit) noexcept {
  setSteps({
      PositiveLength(10000),    // 0.01mm
      PositiveLength(25400),    // 0.0254mm
      PositiveLength(100000),   // 0.1mm
      PositiveLength(254000),   // 0.254mm
      PositiveLength(1000000),  // 1mm
      PositiveLength(2540000),  // 2.54mm
  });
  setUnit(unit);
}

void LengthEditBase::configureForTextHeight() noexcept {
  setSteps({
      PositiveLength(100000),  // 0.1mm
      PositiveLength(254000),  // 0.254mm
      PositiveLength(500000),  // 0.5mm (default)
  });

  // For the text height, always use millimeters, no matter what unit is set.
  // This makes it easier to follow library conventions since they specify the
  // text height in millimeters.
  setUnit(LengthUnit::millimeters());
}

void LengthEditBase::configureForTextStrokeWidth() noexcept {
  setSteps({
      PositiveLength(100000),  // 0.1mm
      PositiveLength(254000),  // 0.254mm
      PositiveLength(500000),  // 0.5mm (default)
  });

  // For the text stroke width, always use millimeters, no matter what unit is
  // set. This makes it easier to follow library conventions since they specify
  // the text stroke width in millimeters.
  setUnit(LengthUnit::millimeters());
}

void LengthEditBase::configureForPinLength(const LengthUnit& unit) noexcept {
  setSteps({
      PositiveLength(2500000),  // 2.5mm (for metric symbols)
      PositiveLength(2540000),  // 2.54mm (default)
  });
  setUnit(unit);
}

void LengthEditBase::configureForDrillDiameter(
    const LengthUnit& unit) noexcept {
  setSteps({
      PositiveLength(254000),  // 0.254mm (for imperial drills)
      PositiveLength(100000),  // 0.1mm (default, for metric drills)
  });
  setUnit(unit);
}

void LengthEditBase::configureForClearance(const LengthUnit& unit) noexcept {
  setSteps({
      PositiveLength(10000),    // 0.01mm
      PositiveLength(25400),    // 0.0254mm
      PositiveLength(100000),   // 0.1mm
      PositiveLength(254000),   // 0.254mm
      PositiveLength(1000000),  // 1mm
      PositiveLength(2540000),  // 2.54mm
  });
  setUnit(unit);
}

/*******************************************************************************
 *  Reimplemented Methods
 ******************************************************************************/

QSize LengthEditBase::minimumSizeHint() const {
  return QAbstractSpinBox::minimumSizeHint() + mAdditionalSize;
}

QSize LengthEditBase::sizeHint() const {
  return QAbstractSpinBox::sizeHint() + mAdditionalSize;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

QAbstractSpinBox::StepEnabled LengthEditBase::stepEnabled() const {
  QAbstractSpinBox::StepEnabled enabled = QAbstractSpinBox::StepNone;
  if ((mSingleStepUp > 0) && (mValue < mMaximum)) {
    enabled |= QAbstractSpinBox::StepUpEnabled;
  }
  if ((mSingleStepDown > 0) && (mValue > mMinimum)) {
    enabled |= QAbstractSpinBox::StepDownEnabled;
  }
  return enabled;
}

void LengthEditBase::stepBy(int steps) {
  if ((mSingleStepUp > 0) && (steps > 0)) {
    setValueImpl(mValue + mSingleStepUp * steps);
  } else if ((mSingleStepDown > 0) && (steps < 0)) {
    setValueImpl(mValue + mSingleStepDown * steps);
  }
}

void LengthEditBase::setValueImpl(Length value) noexcept {
  // Always clip the value to the allowed range! Otherwise the value might not
  // be convertible into the constrained Length type of derived classes!
  value = qBound(mMinimum, value, mMaximum);

  // To avoid unnecessary clearing the QLineEdit selection, only update the
  // value (and therefore the text) if really needed.
  if (value != mValue) {
    mValue = value;
    updateSingleStep();
    updateText();
    valueChangedImpl();
    update();  // step buttons might need to be repainted
  }
}

void LengthEditBase::updateValueFromText(QString text) noexcept {
  try {
    LengthUnit         unit   = extractUnitFromExpression(text);
    MathParser::Result result = MathParser().parse(text);
    if (result.valid) {
      Length value = unit.convertFromUnit(result.value);  // can throw
      // Only accept values in the allowed range.
      if ((value >= mMinimum) && (value <= mMaximum)) {
        mValue = value;
        mUnit  = unit;
        updateSingleStep();
        // In contrast to setValueImpl(), do NOT call updateText() to avoid
        // disturbing the user while writing the text!
        valueChangedImpl();
        update();  // step buttons might need to be repainted
      } else {
        qWarning() << "LengthEditBase: Entered text was a valid number, but "
                      "outside the allowed range.";
      }
    }
  } catch (const Exception&) {
    qWarning() << "LengthEditBase: Entered text was a valid expression, but "
                  "evaluated to an invalid number:"
               << text;
  }
}

void LengthEditBase::updateSingleStep() noexcept {
  if ((mValue == 0) || (mValue == mMinimum)) {
    return;  // keep last step values
  }

  Length up;
  Length down;
  foreach (const PositiveLength& step, mSteps) {
    if ((mValue % (*step)) == 0) {
      up = *step;
      if ((mValue.abs() > (*step)) || (down == 0)) {
        down = *step;
      }
    }
  }
  if (mValue < 0) {
    std::swap(up, down);
  }
  // Do not allow to step down if it would lead in a value smaller than the
  // minimum. This is needed for PositiveLengthEdit to avoid e.g. the next lower
  // value of 0.1mm would be 0.000001mm because it gets clipped to the minimum.
  if ((down > 0) && (mValue < (mMinimum + down))) {
    down = 0;
  }

  mSingleStepUp   = up;
  mSingleStepDown = down;
}

void LengthEditBase::updateText() noexcept {
  lineEdit()->setText(getValueStr(mUnit));
}

LengthUnit LengthEditBase::extractUnitFromExpression(QString& expression) const
    noexcept {
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    foreach (const QString& suffix, unit.getUserInputSuffixes()) {
      if (expression.endsWith(suffix)) {
        expression.chop(suffix.length());
        return unit;
      }
    }
  }
  return mUnit;
}

void LengthEditBase::changeUnitActionTriggered() noexcept {
  QMenu        menu(this);
  QActionGroup group(&menu);
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    QString text = getValueStr(unit);
    if (unit == LengthUnit::nanometers()) {
      text += " (" % tr("internal") % ")";
    }
    QAction* action = menu.addAction(text);
    group.addAction(action);
    action->setCheckable(true);
    action->setChecked(unit == mUnit);
    connect(action, &QAction::triggered, [this, unit]() { setUnit(unit); });
  }
  menu.exec(QCursor::pos());
}

QString LengthEditBase::getValueStr(const LengthUnit& unit) const noexcept {
  if (unit == LengthUnit::nanometers()) {
    return QString::number(mValue.toNm()) % " " % unit.toShortStringTr();
  } else {
    // Show only a limited number of decimals to avoid very odd numbers with
    // many decimals due to converting between different units (e.g. a value
    // of 0.1mm displayed in mils is 3.937007874, but such a number is annoying
    // in a GUI). The underlying value is of course not truncated, so it should
    // be fine to reduce the displayed number of decimals.
    return Toolbox::floatToString(unit.convertToUnit(mValue),
                                  unit.getReasonableNumberOfDecimals(),
                                  locale()) %
           " " % unit.toShortStringTr();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
