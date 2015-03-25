/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef LIBRARY_LIBRARYELEMENTATTRIBUTE_H
#define LIBRARY_LIBRARYELEMENTATTRIBUTE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../common/exceptions.h"
#include "../common/file_io/if_xmlserializableobject.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class AttributeType;
class AttributeUnit;

namespace library {
class GenericComponent;
}

/*****************************************************************************************
 *  Class LibraryElementAttribute
 ****************************************************************************************/

namespace library {

/**
 * @brief The LibraryElementAttribute class represents an attribute of a library element
 */
class LibraryElementAttribute final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Attribute)

    public:

        // Constructors / Destructor
        explicit LibraryElementAttribute(const XmlDomElement& domElement) throw (Exception);
        ~LibraryElementAttribute() noexcept;


        // Getters
        const QString& getKey() const noexcept {return mKey;}
        const AttributeType& getType() const noexcept {return *mType;}
        const AttributeUnit* getDefaultUnit() const noexcept {return mDefaultUnit;}
        QString getName(const QStringList& localeOrder = QStringList()) const noexcept;
        QString getDescription(const QStringList& localeOrder = QStringList()) const noexcept;
        QString getDefaultValue(const QStringList& localeOrder = QStringList()) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}
        const QMap<QString, QString>& getDefaultValues() const noexcept {return mDefaultValues;}

        // General Methods
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


    private:

        // make some methods inaccessible...
        LibraryElementAttribute();
        LibraryElementAttribute(const LibraryElementAttribute& other);
        LibraryElementAttribute& operator=(const LibraryElementAttribute& rhs);

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Attributes
        QString mKey;
        const AttributeType* mType;
        const AttributeUnit* mDefaultUnit;
        QMap<QString, QString> mNames;
        QMap<QString, QString> mDescriptions;
        QMap<QString, QString> mDefaultValues;
};

} // namespace library

#endif // LIBRARY_LIBRARYELEMENTATTRIBUTE_H
