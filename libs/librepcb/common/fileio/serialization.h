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

#ifndef LIBREPCB_SERIALIZATION_H
#define LIBREPCB_SERIALIZATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../version.h"
#include "sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

template <typename T>
void serializeToSExpression(SExpression& root, const T& obj);

template <typename T>
void serializeObject(SExpression& root, const T& obj) {
  serializeToSExpression(root, obj);
}

template <typename T>
SExpression serializeObject(const T& obj, const QString& name) {
  SExpression root = SExpression::createList(name);
  serializeObject(root, obj);
  return root;
}

template <typename T>
void serializeObjectContainer(SExpression& root, const T& obj,
                              const QString& itemName) {
  for (const auto& item : obj) {
    root.appendChild(serializeObject(item, itemName), true);
  }
}

template <typename T>
void serializePointerContainer(SExpression& root, const T& obj,
                               const QString& itemName) {
  for (const auto& item : obj) {
    root.appendChild(serializeObject(*item, itemName), true);
  }
}

template <typename T>
void serializePointerContainerUuidSorted(SExpression& root, const T& obj,
                                         const QString& itemName) {
  T copy = obj;
  std::sort(
      copy.begin(), copy.end(),
      [](const typename T::value_type& a, const typename T::value_type& b) {
        return a->getUuid() < b->getUuid();
      });
  serializePointerContainer(root, copy, itemName);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SERIALIZATION_H
