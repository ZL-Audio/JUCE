/*
  ==============================================================================

   This file is part of the JUCE 9 preview.
   Copyright (c) Raw Material Software Limited

   You may use this code under the terms of the AGPLv3
   (see www.gnu.org/licenses).

   For the JUCE 9 preview this file cannot be licensed commercially.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

/**
    A helper struct providing functions for managing sorted containers.

    These helper functions simplify common operations on containers that
    are kept in a sorted order.

    @tags{Core}
*/
struct OrderedContainerHelpers
{
    /** Returns true if neither value compares less than the other.
        This is the same definition of equivalence as used by the std containers "set" and "map".
    */
    template <typename A, typename B, typename Less = std::less<>>
    static constexpr bool equivalent (const A& a, const B& b, Less less = {})
    {
        return ! less (a, b) && ! less (b, a);
    }

    //==============================================================================
    /** Searches for an element in a sorted container using binary search.

        This returns an iterator to the found element, or container.end() if not found.
    */
    template <typename OrderedContainer, typename ValueType, typename Less = std::less<>>
    static auto find (const OrderedContainer& container, const ValueType& valueToFind, Less less = {})
    {
        // This function won't do the right thing on a container that's not sorted!
        jassert (std::is_sorted (container.begin(), container.end(), less));

        auto iter = std::lower_bound (container.begin(), container.end(), valueToFind, less);

        if (iter == container.end() || ! equivalent (*iter, valueToFind, less))
            return container.end();

        return iter;
    }

    /** If the container already contains a value equivalent to the valueToInsert, assigns
        the new value over the old one; otherwise, if no equivalent tag exists, inserts the
        new value preserving the sorted property of the container.
    */
    template <typename OrderedContainer, typename ValueType, typename Less = std::less<>>
    static void insertOrAssign (OrderedContainer& container,
                                const ValueType& valueToInsert,
                                Less less = {})
    {
        // This function won't do the right thing on a container that's not sorted!
        jassert (std::is_sorted (container.begin(), container.end(), less));

        auto iter = std::lower_bound (container.begin(), container.end(), valueToInsert, less);

        if (iter != container.end() && equivalent (*iter, valueToInsert, less))
        {
            *iter = valueToInsert;
        }
        else
        {
            container.insert (iter, valueToInsert);
        }
    }

    /** Removes a specific element from a sorted array, preserving order.

        Searches for an element in the container that compares equivalent to valueToRemove and
        erases it if present, preserving the sorted property of the container.

        Returns true if the array was modified or false otherwise (i.e. the element was not removed).
    */
    template <typename OrderedContainer, typename ValueType, typename Less = std::less<>>
    static bool remove (OrderedContainer& container,
                        const ValueType& valueToRemove,
                        Less less = {})
    {
        auto iter = find (container, valueToRemove, less);

        if (iter == container.end())
            return false;

        container.erase (iter);
        return true;
    }

    /** Searches for an element in a sorted container and returns true if a match is found. */
    template <typename OrderedContainer, typename ValueType, typename Less = std::less<>>
    static bool contains (const OrderedContainer& container, const ValueType& valueToFind, Less less = {})
    {
        return find (container, valueToFind, less) != container.end();
    }

    OrderedContainerHelpers() = delete;
};

}
