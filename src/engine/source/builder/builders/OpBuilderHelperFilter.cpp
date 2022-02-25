/* Copyright (C) 2015-2021, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <string>
#include <tuple>

#include "OpBuilderHelperFilter.hpp"
#include "stringUtils.hpp"

using DocumentValue = builder::internals::types::DocumentValue;
namespace
{

using opString = std::optional<std::string>;
/**
 * @brief Get the Comparator operator, amd the value to compare 
 * or the reference to value to compare
 * 
 * @param def The JSON definition of the operator
 * @return std::tuple<std::string, opString, opString> the operator, 
 * the value to compare and the reference to value to compare (if exists)
 * @throw std::runtime_error if the number of parameters is not valid
 * @throw std::logic_error if the json node is not valid definition for helper function
 */
std::tuple<std::string, opString, opString> getCompOpParameter(const DocumentValue & def)
{
    // Get destination path
    std::string field = def.MemberBegin()->name.GetString();
    // Get function helper
    if (!def.MemberBegin()->value.IsString()) {
        throw std::logic_error("Invalid operator definition");
    }
    std::string rawValue = def.MemberBegin()->value.GetString();

    // Parse parameters
    std::vector<std::string> parameters = utils::string::split(rawValue, '/');
    if (parameters.size() != 2)
    {
        throw std::runtime_error("Invalid number of parameters");
    }

    std::optional<std::string> refValue;
    std::optional<std::string> value;

    if (parameters[1][0] == '$')
    {
        refValue = parameters[1].substr(1);
    }
    else
    {
        value = parameters[1];
    }

    return {field, refValue, value};
}
} // namespace

namespace builder::internals::builders
{

types::Lifter opBuilderHelperExists(const DocumentValue & def)
{
    // Get field
    std::string field = def.MemberBegin()->name.GetString();

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter([=](types::Event e) { return e.exists("/" + field); });
    };
}

types::Lifter opBuilderHelperNotExists(const DocumentValue & def)
{
    // Get field
    std::string field = def.MemberBegin()->name.GetString();

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter([=](types::Event e) { return !e.exists("/" + field); });
    };
}

//*************************************************
//*           String filters                      *
//*************************************************

bool opBuilderHelperStringComparison(const std::string key, char op, types::Event & e,
                                     std::optional<std::string> refValue,
                                     std::optional<std::string> value)
{

    // TODO Remove try catch or if nullptr after fix get method of document class
    // Get value to compare
    const rapidjson::Value * fieldToCompare{};
    try
    {
        fieldToCompare = e.get("/" + key);
    }
    catch (std::exception & e)
    {
        // TODO Check exception type
        return false;
    }

    if (fieldToCompare == nullptr || !fieldToCompare->IsString())
    {
        return false;
    }

    // get str to compare
    if (refValue.has_value())
    {
        // Get reference to json event
        // TODO Remove try catch or if nullptr after fix get method of document class
        const rapidjson::Value * refValueToCheck{};
        try
        {
            refValueToCheck = e.get("/" + refValue.value());
        }
        catch (std::exception & ex)
        {
            // TODO Check exception type
            return false;
        }

        if (refValueToCheck == nullptr || !refValueToCheck->IsString())
        {
            return false;
        }
        value = std::string{refValueToCheck->GetString()};
    }

    // String operation
    switch (op)
    {
        case '=':
            return std::string{fieldToCompare->GetString()} == value.value();
        case '!':
            return std::string{fieldToCompare->GetString()} != value.value();
        case '>':
            return std::string{fieldToCompare->GetString()} > value.value();
        // case '>=':
        case 'g':
            return std::string{fieldToCompare->GetString()} >= value.value();
        case '<':
            return std::string{fieldToCompare->GetString()} < value.value();
        // case '<=':
        case 'l':
            return std::string{fieldToCompare->GetString()} <= value.value();

        default:
            // if raise here, then the logic is wrong
            throw std::invalid_argument("Invalid operator: '" + std::string{op} + "' ");
    }

    return false;
}

// <field>: s_eq/<value>
types::Lifter opBuilderHelperString_eq(const DocumentValue & def)
{
    auto [key, refValue, value] = getCompOpParameter(def);

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter(
            [key, refValue, value](types::Event e)
            {
                // try and catche, return false
                return opBuilderHelperStringComparison(key, '=', e,
                                                       refValue, value);
            });
    };
}

// <field>: s_ne/<value>
types::Lifter opBuilderHelperString_ne(const DocumentValue & def)
{
    auto [key, refValue, value] = getCompOpParameter(def);

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter(
            [key, refValue, value](types::Event e) {
                return opBuilderHelperStringComparison(key, '!', e,
                                                       refValue, value);
            });
    };
}

// <field>: s_gt/<value>|$<ref>
types::Lifter opBuilderHelperString_gt(const DocumentValue & def)
{
    auto [key, refValue, value] = getCompOpParameter(def);

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter(
            [key, refValue, value](types::Event e) {
                return opBuilderHelperStringComparison(key, '>', e,
                                                       refValue, value);
            });
    };
}

// <field>: s_ge/<value>|$<ref>
types::Lifter opBuilderHelperString_ge(const DocumentValue & def)
{
    auto [key, refValue, value] = getCompOpParameter(def);

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter(
            [key, refValue, value](types::Event e) {
                return opBuilderHelperStringComparison(key, 'g', e,
                                                       refValue, value);
            });
    };
}

// <field>: s_lt/<value>|$<ref>
types::Lifter opBuilderHelperString_lt(const DocumentValue & def)
{
    auto [key, refValue, value] = getCompOpParameter(def);

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter(
            [key, refValue, value](types::Event e) {
                return opBuilderHelperStringComparison(key, '<', e,
                                                       refValue, value);
            });
    };
}

// <field>: s_le/<value>|$<ref>
types::Lifter opBuilderHelperString_le(const DocumentValue & def)
{
    auto [key, refValue, value] = getCompOpParameter(def);

    // Return Lifter
    return [=](types::Observable o)
    {
        // Append rxcpp operation
        return o.filter(
            [key, refValue, value](types::Event e) {
                return opBuilderHelperStringComparison(key, 'l', e,
                                                       refValue, value);
            });
    };
}

//*************************************************
//*               Int filters                     *
//*************************************************

} // namespace builder::internals::builders