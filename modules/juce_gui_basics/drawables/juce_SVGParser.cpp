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

#include <juce_gui_basics/drawables/lunasvg/plutovg/source/plutovg-private.h>

#include <stack>

namespace juce
{

class SVGState
{
public:
    //==============================================================================
    explicit SVGState (const XmlElement* topLevel, const File& svgFile = {})
       : originalFile (svgFile), topLevelXml (topLevel, nullptr)
    {
    }

    struct XmlPath
    {
        XmlPath (const XmlElement* e, const XmlPath* p) noexcept : xml (e), parent (p)  {}

        const XmlElement& operator*() const noexcept            { jassert (xml != nullptr); return *xml; }
        const XmlElement* operator->() const noexcept           { return xml; }
        XmlPath getChild (const XmlElement* e) const noexcept   { return XmlPath (e, this); }

        template <typename OperationType>
        bool applyOperationToChildWithID (const String& id, OperationType& op) const
        {
            for (auto* e : xml->getChildIterator())
            {
                XmlPath child (e, this);

                if (e->compareAttribute ("id", id)
                      && ! child->hasTagName ("defs"))
                    return op (child);

                if (child.applyOperationToChildWithID (id, op))
                    return true;
            }

            return false;
        }

        const XmlElement* xml;
        const XmlPath* parent;
    };

    //==============================================================================
    struct UsePathOp
    {
        const SVGState* state;
        Path* targetPath;

        bool operator() (const XmlPath& xmlPath) const
        {
            return state->parsePathElement (xmlPath, *targetPath);
        }
    };

    struct UseTextOp
    {
        const SVGState* state;
        AffineTransform* transform;
        Drawable* target;

        bool operator() (const XmlPath& xmlPath)
        {
            target = state->parseText (xmlPath, true, transform);
            return target != nullptr;
        }
    };

    struct UseImageOp
    {
        const SVGState* state;
        AffineTransform* transform;
        Drawable* target;

        bool operator() (const XmlPath& xmlPath)
        {
            target = state->parseImage (xmlPath, true, transform);
            return target != nullptr;
        }
    };

    struct GetClipPathOp
    {
        SVGState* state;
        Drawable* target;

        bool operator() (const XmlPath& xmlPath)
        {
            return state->applyClipPath (*target, xmlPath);
        }
    };

    struct SetGradientStopsOp
    {
        const SVGState* state;
        ColourGradient* gradient;

        bool operator() (const XmlPath& xml) const
        {
            return state->addGradientStopsIn (*gradient, xml);
        }
    };

    struct GetFillTypeOp
    {
        const SVGState* state;
        const Path* path;
        float opacity;
        FillType fillType;

        bool operator() (const XmlPath& xml)
        {
            if (xml->hasTagNameIgnoringNamespace ("linearGradient")
                 || xml->hasTagNameIgnoringNamespace ("radialGradient"))
            {
                fillType = state->getGradientFillType (xml, *path, opacity);
                return true;
            }

            return false;
        }
    };

    //==============================================================================
    Drawable* parseSVGElement (const XmlPath& xml)
    {
        auto drawable = new DrawableComposite();
        setCommonAttributes (*drawable, xml);

        SVGState newState (*this);

        if (xml->hasAttribute ("transform"))
            newState.addTransform (xml);

        newState.width  = getCoordLength (xml->getStringAttribute ("width",  String (newState.width)),  viewBoxW);
        newState.height = getCoordLength (xml->getStringAttribute ("height", String (newState.height)), viewBoxH);

        if (newState.width  <= 0) newState.width  = 100;
        if (newState.height <= 0) newState.height = 100;

        Point<float> viewboxXY;

        if (xml->hasAttribute ("viewBox"))
        {
            auto viewBoxAtt = xml->getStringAttribute ("viewBox");
            auto viewParams = viewBoxAtt.getCharPointer();
            Point<float> vwh;

            if (parseCoords (viewParams, viewboxXY, true)
                 && parseCoords (viewParams, vwh, true)
                 && vwh.x > 0
                 && vwh.y > 0)
            {
                newState.viewBoxW = vwh.x;
                newState.viewBoxH = vwh.y;

                auto placementFlags = parsePlacementFlags (xml->getStringAttribute ("preserveAspectRatio").trim());

                if (placementFlags != 0)
                    newState.transform = RectanglePlacement (placementFlags)
                                            .getTransformToFit (Rectangle<float> (viewboxXY.x, viewboxXY.y, vwh.x, vwh.y),
                                                                Rectangle<float> (newState.width, newState.height))
                                            .followedBy (newState.transform);
            }
        }
        else
        {
            if (approximatelyEqual (viewBoxW, 0.0f))  newState.viewBoxW = newState.width;
            if (approximatelyEqual (viewBoxH, 0.0f))  newState.viewBoxH = newState.height;
        }

        newState.parseSubElements (xml, *drawable);

        drawable->setContentArea ({ viewboxXY.x, viewboxXY.y, newState.viewBoxW, newState.viewBoxH });
        drawable->resetBoundingBoxToContentArea();

        return drawable;
    }

    //==============================================================================
    void parsePathString (Path& path, const String& pathString) const
    {
        auto d = pathString.getCharPointer().findEndOfWhitespace();

        Point<float> subpathStart, last, last2, p1, p2, p3;
        juce_wchar currentCommand = 0, previousCommand = 0;
        bool isRelative = true;
        bool carryOn = true;

        while (! d.isEmpty())
        {
            if (CharPointer_ASCII ("MmLlHhVvCcSsQqTtAaZz").indexOf (*d) >= 0)
            {
                currentCommand = d.getAndAdvance();
                isRelative = currentCommand >= 'a';
            }

            switch (currentCommand)
            {
            case 'M':
            case 'm':
            case 'L':
            case 'l':
                if (parseCoordsOrSkip (d, p1, false))
                {
                    if (isRelative)
                        p1 += last;

                    if (currentCommand == 'M' || currentCommand == 'm')
                    {
                        subpathStart = p1;
                        path.startNewSubPath (p1);
                        currentCommand = 'l';
                    }
                    else
                        path.lineTo (p1);

                    last2 = last = p1;
                }
                break;

            case 'H':
            case 'h':
                if (parseCoord (d, p1.x, false, Axis::x))
                {
                    if (isRelative)
                        p1.x += last.x;

                    path.lineTo (p1.x, last.y);

                    last2.x = last.x;
                    last.x = p1.x;
                }
                else
                {
                    ++d;
                }
                break;

            case 'V':
            case 'v':
                if (parseCoord (d, p1.y, false, Axis::y))
                {
                    if (isRelative)
                        p1.y += last.y;

                    path.lineTo (last.x, p1.y);

                    last2.y = last.y;
                    last.y = p1.y;
                }
                else
                {
                    ++d;
                }
                break;

            case 'C':
            case 'c':
                if (parseCoordsOrSkip (d, p1, false)
                     && parseCoordsOrSkip (d, p2, false)
                     && parseCoordsOrSkip (d, p3, false))
                {
                    if (isRelative)
                    {
                        p1 += last;
                        p2 += last;
                        p3 += last;
                    }

                    path.cubicTo (p1, p2, p3);

                    last2 = p2;
                    last = p3;
                }
                break;

            case 'S':
            case 's':
                if (parseCoordsOrSkip (d, p1, false)
                     && parseCoordsOrSkip (d, p3, false))
                {
                    if (isRelative)
                    {
                        p1 += last;
                        p3 += last;
                    }

                    p2 = last;

                    if (CharPointer_ASCII ("CcSs").indexOf (previousCommand) >= 0)
                        p2 += (last - last2);

                    path.cubicTo (p2, p1, p3);

                    last2 = p1;
                    last = p3;
                }
                break;

            case 'Q':
            case 'q':
                if (parseCoordsOrSkip (d, p1, false)
                     && parseCoordsOrSkip (d, p2, false))
                {
                    if (isRelative)
                    {
                        p1 += last;
                        p2 += last;
                    }

                    path.quadraticTo (p1, p2);

                    last2 = p1;
                    last = p2;
                }
                break;

            case 'T':
            case 't':
                if (parseCoordsOrSkip (d, p1, false))
                {
                    if (isRelative)
                        p1 += last;

                    p2 = last;

                    if (CharPointer_ASCII ("QqTt").indexOf (previousCommand) >= 0)
                        p2 += (last - last2);

                    path.quadraticTo (p2, p1);

                    last2 = p2;
                    last = p1;
                }
                break;

            case 'A':
            case 'a':
                if (parseCoordsOrSkip (d, p1, false))
                {
                    String num;
                    bool flagValue = false;

                    if (parseNextNumber (d, num, false))
                    {
                        auto angle = degreesToRadians (parseSafeFloat (num));

                        if (parseNextFlag (d, flagValue))
                        {
                            auto largeArc = flagValue;

                            if (parseNextFlag (d, flagValue))
                            {
                                auto sweep = flagValue;

                                if (parseCoordsOrSkip (d, p2, false))
                                {
                                    if (isRelative)
                                        p2 += last;

                                    if (last != p2)
                                    {
                                        double centreX, centreY, startAngle, deltaAngle;
                                        double rx = p1.x, ry = p1.y;

                                        endpointToCentreParameters (last.x, last.y, p2.x, p2.y,
                                                                    angle, largeArc, sweep,
                                                                    rx, ry, centreX, centreY,
                                                                    startAngle, deltaAngle);

                                        path.addCentredArc ((float) centreX, (float) centreY,
                                                            (float) rx, (float) ry,
                                                            angle, (float) startAngle, (float) (startAngle + deltaAngle),
                                                            false);

                                        path.lineTo (p2);
                                    }

                                    last2 = last;
                                    last = p2;
                                }
                            }
                        }
                    }
                }

                break;

            case 'Z':
            case 'z':
                path.closeSubPath();
                last = last2 = subpathStart;
                d.incrementToEndOfWhitespace();
                currentCommand = 'M';
                break;

            default:
                carryOn = false;
                break;
            }

            if (! carryOn)
                break;

            previousCommand = currentCommand;
        }

        // paths that finish back at their start position often seem to be
        // left without a 'z', so need to be closed explicitly
        if (path.getCurrentPosition() == subpathStart)
            path.closeSubPath();
    }

private:
    //==============================================================================
    const File originalFile;
    const XmlPath topLevelXml;
    float width = 512, height = 512, viewBoxW = 0, viewBoxH = 0;
    AffineTransform transform;
    String cssStyleText;

    static bool isNone (const String& s) noexcept
    {
        return s.equalsIgnoreCase ("none");
    }

    static void setCommonAttributes (Drawable& d, const XmlPath& xml)
    {
        auto compID = xml->getStringAttribute ("id");
        d.setName (compID);
    }

    //==============================================================================
    void parseSubElements (const XmlPath& xml, DrawableComposite& parentDrawable, bool shouldParseClip = true)
    {
        for (auto* e : xml->getChildIterator())
        {
            const XmlPath child (xml.getChild (e));

            if (auto* drawable = parseSubElement (child))
            {
                parentDrawable.addChildComponent (drawable);

                if (shouldParseClip)
                    parseClipPath (child, *drawable);
            }
        }
    }

    Drawable* parseSubElement (const XmlPath& xml)
    {
        {
            Path path;
            if (parsePathElement (xml, path))
                return parseShape (xml, path);
        }

        auto tag = xml->getTagNameWithoutNamespace();

        if (tag == "g")         return parseGroupElement (xml, true);
        if (tag == "svg")       return parseSVGElement (xml);
        if (tag == "text")      return parseText (xml, true, nullptr);
        if (tag == "image")     return parseImage (xml, true);
        if (tag == "switch")    return parseSwitch (xml);
        if (tag == "a")         return parseLinkElement (xml);
        if (tag == "use")       return parseUseOther (xml);
        if (tag == "style")     parseCSSStyle (xml);
        if (tag == "defs")      parseDefs (xml);

        return nullptr;
    }

    bool parsePathElement (const XmlPath& xml, Path& path) const
    {
        auto tag = xml->getTagNameWithoutNamespace();

        if (tag == "path")      { parsePath (xml, path);           return true; }
        if (tag == "rect")      { parseRect (xml, path);           return true; }
        if (tag == "circle")    { parseCircle (xml, path);         return true; }
        if (tag == "ellipse")   { parseEllipse (xml, path);        return true; }
        if (tag == "line")      { parseLine (xml, path);           return true; }
        if (tag == "polyline")  { parsePolygon (xml, true, path);  return true; }
        if (tag == "polygon")   { parsePolygon (xml, false, path); return true; }
        if (tag == "use")       { return parseUsePath (xml, path); }

        return false;
    }

    DrawableComposite* parseSwitch (const XmlPath& xml)
    {
        if (auto* group = xml->getChildByName ("g"))
            return parseGroupElement (xml.getChild (group), true);

        return nullptr;
    }

    DrawableComposite* parseGroupElement (const XmlPath& xml, bool shouldParseTransform)
    {
        if (shouldParseTransform && xml->hasAttribute ("transform"))
        {
            SVGState newState (*this);
            newState.addTransform (xml);

            return newState.parseGroupElement (xml, false);
        }

        auto* drawable = new DrawableComposite();
        setCommonAttributes (*drawable, xml);
        parseSubElements (xml, *drawable);

        drawable->resetContentAreaAndBoundingBoxToFitChildren();
        return drawable;
    }

    DrawableComposite* parseLinkElement (const XmlPath& xml)
    {
        return parseGroupElement (xml, true); // TODO: support for making this clickable
    }

    //==============================================================================
    void parsePath (const XmlPath& xml, Path& path) const
    {
        parsePathString (path, xml->getStringAttribute ("d"));

        if (getStyleAttribute (xml, "fill-rule").trim().equalsIgnoreCase ("evenodd"))
            path.setUsingNonZeroWinding (false);
    }

    void parseRect (const XmlPath& xml, Path& rect) const
    {
        const bool hasRX = xml->hasAttribute ("rx");
        const bool hasRY = xml->hasAttribute ("ry");

        if (hasRX || hasRY)
        {
            float rx = getCoordLength (xml, "rx", viewBoxW);
            float ry = getCoordLength (xml, "ry", viewBoxH);

            if (! hasRX)
                rx = ry;
            else if (! hasRY)
                ry = rx;

            rect.addRoundedRectangle (getCoordLength (xml, "x", viewBoxW),
                                      getCoordLength (xml, "y", viewBoxH),
                                      getCoordLength (xml, "width", viewBoxW),
                                      getCoordLength (xml, "height", viewBoxH),
                                      rx, ry);
        }
        else
        {
            rect.addRectangle (getCoordLength (xml, "x", viewBoxW),
                               getCoordLength (xml, "y", viewBoxH),
                               getCoordLength (xml, "width", viewBoxW),
                               getCoordLength (xml, "height", viewBoxH));
        }
    }

    void parseCircle (const XmlPath& xml, Path& circle) const
    {
        auto cx = getCoordLength (xml, "cx", viewBoxW);
        auto cy = getCoordLength (xml, "cy", viewBoxH);
        auto radius = getCoordLength (xml, "r", viewBoxW);

        circle.addEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }

    void parseEllipse (const XmlPath& xml, Path& ellipse) const
    {
        auto cx      = getCoordLength (xml, "cx", viewBoxW);
        auto cy      = getCoordLength (xml, "cy", viewBoxH);
        auto radiusX = getCoordLength (xml, "rx", viewBoxW);
        auto radiusY = getCoordLength (xml, "ry", viewBoxH);

        ellipse.addEllipse (cx - radiusX, cy - radiusY, radiusX * 2.0f, radiusY * 2.0f);
    }

    void parseLine (const XmlPath& xml, Path& line) const
    {
        auto x1 = getCoordLength (xml, "x1", viewBoxW);
        auto y1 = getCoordLength (xml, "y1", viewBoxH);
        auto x2 = getCoordLength (xml, "x2", viewBoxW);
        auto y2 = getCoordLength (xml, "y2", viewBoxH);

        line.startNewSubPath (x1, y1);
        line.lineTo (x2, y2);
    }

    void parsePolygon (const XmlPath& xml, bool isPolyline, Path& path) const
    {
        auto pointsAtt = xml->getStringAttribute ("points");
        auto points = pointsAtt.getCharPointer();
        Point<float> p;

        if (parseCoords (points, p, true))
        {
            Point<float> first (p), last;

            path.startNewSubPath (first);

            while (parseCoords (points, p, true))
            {
                last = p;
                path.lineTo (p);
            }

            if ((! isPolyline) || first == last)
                path.closeSubPath();
        }
    }

    static String getLinkedID (const XmlPath& xml)
    {
        auto link = xml->getStringAttribute ("xlink:href");

        if (link.startsWithChar ('#'))
            return link.substring (1);

        return {};
    }

    bool parseUsePath (const XmlPath& xml, Path& path) const
    {
        auto linkedID = getLinkedID (xml);

        if (linkedID.isNotEmpty())
        {
            UsePathOp op = { this, &path };
            return topLevelXml.applyOperationToChildWithID (linkedID, op);
        }

        return false;
    }

    Drawable* parseUseOther (const XmlPath& xml) const
    {
        if (auto* drawableText  = parseText (xml, false, nullptr))    return drawableText;
        if (auto* drawableImage = parseImage (xml, false))   return drawableImage;

        return nullptr;
    }

    static String parseURL (const String& str)
    {
        if (str.startsWithIgnoreCase ("url"))
            return str.fromFirstOccurrenceOf ("#", false, false)
                      .upToLastOccurrenceOf (")", false, false).trim();

        return {};
    }

    //==============================================================================
    Drawable* parseShape (const XmlPath& xml, Path& path,
                          bool shouldParseTransform = true,
                          AffineTransform* additonalTransform = nullptr) const
    {
        if (shouldParseTransform && xml->hasAttribute ("transform"))
        {
            SVGState newState (*this);
            newState.addTransform (xml);

            return newState.parseShape (xml, path, false, additonalTransform);
        }

        auto dp = new DrawablePath();
        setCommonAttributes (*dp, xml);
        dp->setFill (Colours::transparentBlack);

        path.applyTransform (transform);

        if (additonalTransform != nullptr)
            path.applyTransform (*additonalTransform);

        dp->setPath (path);

        dp->setFill (getPathFillType (path, xml, "fill",
                                      getStyleAttribute (xml, "fill-opacity"),
                                      getStyleAttribute (xml, "opacity"),
                                      pathContainsClosedSubPath (path) ? Colours::black
                                                                       : Colours::transparentBlack));

        auto strokeType = getStyleAttribute (xml, "stroke");

        if (strokeType.isNotEmpty() && ! isNone (strokeType))
        {
            dp->setStrokeFill (getPathFillType (path, xml, "stroke",
                                                getStyleAttribute (xml, "stroke-opacity"),
                                                getStyleAttribute (xml, "opacity"),
                                                Colours::transparentBlack));

            dp->setStrokeType (getStrokeFor (xml));
        }

        auto strokeDashArray = getStyleAttribute (xml, "stroke-dasharray");

        if (strokeDashArray.isNotEmpty())
            parseDashArray (strokeDashArray, *dp);

        return dp;
    }

    static bool pathContainsClosedSubPath (const Path& path) noexcept
    {
        for (Path::Iterator iter (path); iter.next();)
            if (iter.elementType == Path::Iterator::closePath)
                return true;

        return false;
    }

    void parseDashArray (const String& dashList, DrawablePath& dp) const
    {
        if (dashList.equalsIgnoreCase ("null") || isNone (dashList))
            return;

        Array<float> dashLengths;

        for (auto t = dashList.getCharPointer();;)
        {
            float value;
            if (! parseCoord (t, value, true, Axis::x))
                break;

            dashLengths.add (value);

            t.incrementToEndOfWhitespace();

            if (*t == ',')
                ++t;
        }

        if (dashLengths.size() > 0)
        {
            auto* dashes = dashLengths.getRawDataPointer();

            for (int i = 0; i < dashLengths.size(); ++i)
            {
                if (dashes[i] <= 0)  // SVG uses zero-length dashes to mean a dotted line
                {
                    if (dashLengths.size() == 1)
                        return;

                    const float nonZeroLength = 0.001f;
                    dashes[i] = nonZeroLength;

                    const int pairedIndex = i ^ 1;

                    if (isPositiveAndBelow (pairedIndex, dashLengths.size())
                          && dashes[pairedIndex] > nonZeroLength)
                        dashes[pairedIndex] -= nonZeroLength;
                }
            }

            dp.setDashLengths (Span<const float> (dashLengths.data(), (size_t) dashLengths.size()));
        }
    }

    bool parseClipPath (const XmlPath& xml, Drawable& d)
    {
        const String clipPath (getStyleAttribute (xml, "clip-path"));

        if (clipPath.isNotEmpty())
        {
            auto urlID = parseURL (clipPath);

            if (urlID.isNotEmpty())
            {
                GetClipPathOp op = { this, &d };
                return topLevelXml.applyOperationToChildWithID (urlID, op);
            }
        }

        return false;
    }

    bool applyClipPath (Drawable& target, const XmlPath& xmlPath)
    {
        if (xmlPath->hasTagNameIgnoringNamespace ("clipPath"))
        {
            std::unique_ptr<DrawableComposite> drawableClipPath (new DrawableComposite());

            parseSubElements (xmlPath, *drawableClipPath, false);

            if (drawableClipPath->getNumChildren() > 0)
            {
                setCommonAttributes (*drawableClipPath, xmlPath);
                target.setClipPath (std::move (drawableClipPath));
                return true;
            }
        }

        return false;
    }

    bool addGradientStopsIn (ColourGradient& cg, const XmlPath& fillXml) const
    {
        bool result = false;

        if (fillXml.xml != nullptr)
        {
            for (auto* e : fillXml->getChildWithTagNameIterator ("stop"))
            {
                auto col = parseColour (fillXml.getChild (e), "stop-color", Colours::black);

                auto opacity = getStyleAttribute (fillXml.getChild (e), "stop-opacity", "1");
                col = col.withMultipliedAlpha (jlimit (0.0f, 1.0f, parseSafeFloat (opacity)));

                auto offset = parseSafeFloat (e->getStringAttribute ("offset"));

                if (e->getStringAttribute ("offset").containsChar ('%'))
                    offset *= 0.01f;

                cg.addColour (jlimit (0.0f, 1.0f, offset), col);
                result = true;
            }
        }

        return result;
    }

    FillType getGradientFillType (const XmlPath& fillXml,
                                  const Path& path,
                                  const float opacity) const
    {
        ColourGradient gradient;

        {
            auto linkedID = getLinkedID (fillXml);

            if (linkedID.isNotEmpty())
            {
                SetGradientStopsOp op = { this, &gradient, };
                topLevelXml.applyOperationToChildWithID (linkedID, op);
            }
        }

        addGradientStopsIn (gradient, fillXml);

        if (int numColours = gradient.getNumColours())
        {
            if (gradient.getColourPosition (0) > 0)
                gradient.addColour (0.0, gradient.getColour (0));

            if (gradient.getColourPosition (numColours - 1) < 1.0)
                gradient.addColour (1.0, gradient.getColour (numColours - 1));
        }
        else
        {
            gradient.addColour (0.0, Colours::black);
            gradient.addColour (1.0, Colours::black);
        }

        if (opacity < 1.0f)
            gradient.multiplyOpacity (opacity);

        jassert (gradient.getNumColours() > 0);

        gradient.isRadial = fillXml->hasTagNameIgnoringNamespace ("radialGradient");

        float gradientWidth = viewBoxW;
        float gradientHeight = viewBoxH;
        float dx = 0.0f;
        float dy = 0.0f;

        const bool userSpace = fillXml->getStringAttribute ("gradientUnits").equalsIgnoreCase ("userSpaceOnUse");

        if (! userSpace)
        {
            auto bounds = path.getBounds();
            dx = bounds.getX();
            dy = bounds.getY();
            gradientWidth = bounds.getWidth();
            gradientHeight = bounds.getHeight();
        }

        if (gradient.isRadial)
        {
            if (userSpace)
                gradient.point1.setXY (dx + getCoordLength (fillXml->getStringAttribute ("cx", "50%"), gradientWidth),
                                       dy + getCoordLength (fillXml->getStringAttribute ("cy", "50%"), gradientHeight));
            else
                gradient.point1.setXY (dx + gradientWidth  * getCoordLength (fillXml->getStringAttribute ("cx", "50%"), 1.0f),
                                       dy + gradientHeight * getCoordLength (fillXml->getStringAttribute ("cy", "50%"), 1.0f));

            auto radius = getCoordLength (fillXml->getStringAttribute ("r", "50%"), gradientWidth);
            gradient.point2 = gradient.point1 + Point<float> (radius, 0.0f);

            //xxx (the fx, fy focal point isn't handled properly here)
        }
        else
        {
            if (userSpace)
            {
                gradient.point1.setXY (dx + getCoordLength (fillXml->getStringAttribute ("x1", "0%"), gradientWidth),
                                       dy + getCoordLength (fillXml->getStringAttribute ("y1", "0%"), gradientHeight));

                gradient.point2.setXY (dx + getCoordLength (fillXml->getStringAttribute ("x2", "100%"), gradientWidth),
                                       dy + getCoordLength (fillXml->getStringAttribute ("y2", "0%"), gradientHeight));
            }
            else
            {
                gradient.point1.setXY (dx + gradientWidth  * getCoordLength (fillXml->getStringAttribute ("x1", "0%"), 1.0f),
                                       dy + gradientHeight * getCoordLength (fillXml->getStringAttribute ("y1", "0%"), 1.0f));

                gradient.point2.setXY (dx + gradientWidth  * getCoordLength (fillXml->getStringAttribute ("x2", "100%"), 1.0f),
                                       dy + gradientHeight * getCoordLength (fillXml->getStringAttribute ("y2", "0%"), 1.0f));
            }

            if (gradient.point1 == gradient.point2)
                return Colour (gradient.getColour (gradient.getNumColours() - 1));
        }

        FillType type (gradient);

        auto gradientTransform = parseTransform (fillXml->getStringAttribute ("gradientTransform"));

        if (gradient.isRadial)
        {
            type.transform = gradientTransform;
        }
        else
        {
            // Transform the perpendicular vector into the new coordinate space for the gradient.
            // This vector is now the slope of the linear gradient as it should appear in the new coord space
            auto perpendicular = Point<float> (gradient.point2.y - gradient.point1.y,
                                               gradient.point1.x - gradient.point2.x)
                                    .transformedBy (gradientTransform.withAbsoluteTranslation (0, 0));

            auto newGradPoint1 = gradient.point1.transformedBy (gradientTransform);
            auto newGradPoint2 = gradient.point2.transformedBy (gradientTransform);

            // Project the transformed gradient vector onto the transformed slope of the linear
            // gradient as it should appear in the new coordinate space
            const float scale = perpendicular.getDotProduct (newGradPoint2 - newGradPoint1)
                                  / perpendicular.getDotProduct (perpendicular);

            type.gradient->point1 = newGradPoint1;
            type.gradient->point2 = newGradPoint2 - perpendicular * scale;
        }

        return type;
    }

    FillType getPathFillType (const Path& path,
                              const XmlPath& xml,
                              StringRef fillAttribute,
                              const String& fillOpacity,
                              const String& overallOpacity,
                              const Colour defaultColour) const
    {
        float opacity = 1.0f;

        if (overallOpacity.isNotEmpty())
            opacity = jlimit (0.0f, 1.0f, parseSafeFloat (overallOpacity));

        if (fillOpacity.isNotEmpty())
            opacity *= jlimit (0.0f, 1.0f, parseSafeFloat (fillOpacity));

        String fill (getStyleAttribute (xml, fillAttribute));
        String urlID = parseURL (fill);

        if (urlID.isNotEmpty())
        {
            GetFillTypeOp op = { this, &path, opacity, FillType() };

            if (topLevelXml.applyOperationToChildWithID (urlID, op))
                return op.fillType;
        }

        if (isNone (fill))
            return Colours::transparentBlack;

        return parseColour (xml, fillAttribute, defaultColour).withMultipliedAlpha (opacity);
    }

    static PathStrokeType::JointStyle getJointStyle (const String& join) noexcept
    {
        if (join.equalsIgnoreCase ("round"))  return PathStrokeType::curved;
        if (join.equalsIgnoreCase ("bevel"))  return PathStrokeType::beveled;

        return PathStrokeType::mitered;
    }

    static PathStrokeType::EndCapStyle getEndCapStyle (const String& cap) noexcept
    {
        if (cap.equalsIgnoreCase ("round"))   return PathStrokeType::rounded;
        if (cap.equalsIgnoreCase ("square"))  return PathStrokeType::square;

        return PathStrokeType::butt;
    }

    float getStrokeWidth (const String& strokeWidth) const noexcept
    {
        auto transformScale = std::sqrt (std::abs (transform.getDeterminant()));
        return transformScale * getCoordLength (strokeWidth, viewBoxW);
    }

    PathStrokeType getStrokeFor (const XmlPath& xml) const
    {
        return PathStrokeType (getStrokeWidth (getStyleAttribute (xml, "stroke-width", "1")),
                               getJointStyle  (getStyleAttribute (xml, "stroke-linejoin")),
                               getEndCapStyle (getStyleAttribute (xml, "stroke-linecap")));
    }

    //==============================================================================
    Drawable* useText (const XmlPath& xml) const
    {
        auto translation = AffineTransform::translation (parseSafeFloat (xml->getStringAttribute ("x")),
                                                         parseSafeFloat (xml->getStringAttribute ("y")));

        UseTextOp op = { this, &translation, nullptr };

        auto linkedID = getLinkedID (xml);

        if (linkedID.isNotEmpty())
            topLevelXml.applyOperationToChildWithID (linkedID, op);

        return op.target;
    }

    /*  Handling the stateful consumption of x and y coordinates added to <text> and <tspan> elements.

        <text> elements must have their own x and y attributes, or be positioned at (0, 0) since groups
        enclosing <text> elements can't have x and y attributes.

        <tspan> elements can be embedded inside <text> elements, and <tspan> elements. <text> elements
        can't be embedded inside <text> or <tspan> elements.

        A <tspan> element can have its own x, y attributes, which it will consume at the same time as
        it consumes its parent's attributes. Its own elements will take precedence, but parent elements
        will be consumed regardless.
    */
    class StringLayoutState
    {
    public:
        StringLayoutState (StringLayoutState* parentIn, Array<float> xIn, Array<float> yIn)
            : parent (parentIn),
              xCoords (std::move (xIn)),
              yCoords (std::move (yIn))
        {
        }

        Point<float> getNextStartingPos() const
        {
            if (parent != nullptr)
                return parent->getNextStartingPos();

            return nextStartingPos;
        }

        void setNextStartingPos (Point<float> newPos)
        {
            nextStartingPos = newPos;

            if (parent != nullptr)
                parent->setNextStartingPos (newPos);
        }

        std::pair<std::optional<float>, std::optional<float>> popCoords()
        {
            auto x = xCoords.isEmpty() ? std::optional<float>{} : std::make_optional (xCoords.removeAndReturn (0));
            auto y = yCoords.isEmpty() ? std::optional<float>{} : std::make_optional (yCoords.removeAndReturn (0));

            if (parent != nullptr)
            {
                auto [parentX, parentY] = parent->popCoords();

                if (! x.has_value())
                    x = parentX;

                if (! y.has_value())
                    y = parentY;
            }

            return { x, y };
        }

        bool hasMoreCoords() const
        {
            if (! xCoords.isEmpty() || ! yCoords.isEmpty())
                return true;

            if (parent != nullptr)
                return parent->hasMoreCoords();

            return false;
        }

        bool getPreviousTokenEndedWithSpace() const
        {
            return previousTokenEndedWithSpace;
        }

        void setPreviousTokenEndedWithSpace (bool endedWithSpace)
        {
            previousTokenEndedWithSpace = endedWithSpace;

            if (parent != nullptr)
                parent->setPreviousTokenEndedWithSpace (endedWithSpace);
        }

    private:
        StringLayoutState* parent = nullptr;
        Point<float> nextStartingPos;
        Array<float> xCoords, yCoords;
        bool previousTokenEndedWithSpace = false;
    };

    Drawable* parseText (const XmlPath& xml, bool shouldParseTransform,
                         AffineTransform* additonalTransform,
                         StringLayoutState* parentLayoutState = nullptr) const
    {
        if (shouldParseTransform && xml->hasAttribute ("transform"))
        {
            SVGState newState (*this);
            newState.addTransform (xml);

            return newState.parseText (xml, false, additonalTransform);
        }

        if (xml->hasTagName ("use"))
            return useText (xml);

        if (! xml->hasTagName ("text") && ! xml->hasTagNameIgnoringNamespace ("tspan"))
            return nullptr;

        // If a <tspan> element has no x, or y attributes of its own, it can still use the
        // parent's yet unconsumed such attributes.
        StringLayoutState layoutState { parentLayoutState,
                                        getCoordList (*xml, Axis::x),
                                        getCoordList (*xml, Axis::y) };

        auto font = getFont (xml);
        auto anchorStr = getStyleAttribute (xml, "text-anchor");

        auto dc = new DrawableComposite();
        setCommonAttributes (*dc, xml);

        const auto children = xml->getChildIterator();

        for (auto childIt = children.begin(); childIt != children.end(); ++childIt)
        {
            const auto firstChild = childIt == children.begin();
            const auto lastChild = std::next (childIt) == children.end();
            auto* e = *childIt;

            if (e->isTextElement())
            {
                auto fullText = e->getText().replace ("\r\n", " ").replace ("\n", " ");

                if (layoutState.getPreviousTokenEndedWithSpace())
                    fullText = fullText.trimStart();

                if (xml->hasTagName ("text"))
                {
                    if (firstChild)
                        fullText = fullText.trimStart();
                    else if (lastChild)
                        fullText = fullText.trimEnd();
                }

                const auto collapseSpaces = [] (const String& s)
                {
                    auto tokens = StringArray::fromTokens (s, false);
                    tokens.removeEmptyStrings();
                    auto collapsed = tokens.joinIntoString (" ");

                    if (s.startsWithChar (' '))
                        collapsed = " " + collapsed;

                    if (s.endsWithChar (' '))
                        collapsed += " ";

                    return collapsed;
                };

                fullText = collapseSpaces (fullText);
                layoutState.setPreviousTokenEndedWithSpace (fullText.endsWithChar (' '));

                const auto subtextElements = [&]
                {
                    std::vector<std::tuple<String, std::optional<float>, std::optional<float>>> result;

                    for (auto it = fullText.begin(), end = fullText.end(); it != end;)
                    {
                        const auto pos = layoutState.popCoords();
                        const auto next = layoutState.hasMoreCoords() ? it + 1 : end;
                        result.emplace_back (String (it, next), pos.first, pos.second);
                        it = next;
                    }

                    return result;
                }();

                for (const auto& [text, optX, optY] : subtextElements)
                {
                    auto dt = new DrawableText();
                    dc->addAndMakeVisible (dt);

                    dt->setPreserveWhitespace (true);
                    dt->setText (text);
                    dt->setFont (font, true);

                    if (additonalTransform != nullptr)
                        dt->setDrawableTransform (transform.followedBy (*additonalTransform));
                    else
                        dt->setDrawableTransform (transform);

                    dt->setColour (parseColour (xml, "fill", Colours::black)
                                       .withMultipliedAlpha (parseSafeFloat (getStyleAttribute (xml, "fill-opacity", "1"))));

                    const auto x = optX.value_or (layoutState.getNextStartingPos().getX());
                    const auto y = optY.value_or (layoutState.getNextStartingPos().getY());

                    Rectangle<float> bounds (x, y - font.getAscent(),
                                             GlyphArrangement::getStringWidth (font, text), font.getHeight());

                    if (anchorStr == "middle")   bounds.setX (bounds.getX() - bounds.getWidth() / 2.0f);
                    else if (anchorStr == "end") bounds.setX (bounds.getX() - bounds.getWidth());

                    dt->setBoundingBox (bounds);

                    layoutState.setNextStartingPos ({ bounds.getRight(), y });
                }
            }
            else if (e->hasTagNameIgnoringNamespace ("tspan"))
            {
                dc->addAndMakeVisible (parseText (xml.getChild (e), true, nullptr, &layoutState));
            }
        }

        return dc;
    }

    Font getFont (const XmlPath& xml) const
    {
        Font f { FontOptions{} };
        auto family = getStyleAttribute (xml, "font-family").unquoted();

        if (family.isNotEmpty())
            f.setTypefaceName (family);

        if (getStyleAttribute (xml, "font-style").containsIgnoreCase ("italic"))
            f.setItalic (true);

        if (getStyleAttribute (xml, "font-weight").containsIgnoreCase ("bold"))
            f.setBold (true);

        return f.withPointHeight (getCoordLength (getStyleAttribute (xml, "font-size", "15"), 1.0f));
    }

    //==============================================================================
    Drawable* useImage (const XmlPath& xml) const
    {
        auto translation = AffineTransform::translation (parseSafeFloat (xml->getStringAttribute ("x")),
                                                         parseSafeFloat (xml->getStringAttribute ("y")));

        UseImageOp op = { this, &translation, nullptr };

        auto linkedID = getLinkedID (xml);

        if (linkedID.isNotEmpty())
            topLevelXml.applyOperationToChildWithID (linkedID, op);

        return op.target;
    }

    Drawable* parseImage (const XmlPath& xml, bool shouldParseTransform,
                          AffineTransform* additionalTransform = nullptr) const
    {
        if (shouldParseTransform && xml->hasAttribute ("transform"))
        {
            SVGState newState (*this);
            newState.addTransform (xml);

            return newState.parseImage (xml, false, additionalTransform);
        }

        if (xml->hasTagName ("use"))
            return useImage (xml);

        if (! xml->hasTagName ("image"))
            return nullptr;

        auto link = xml->getStringAttribute ("xlink:href");

        std::unique_ptr<InputStream> inputStream;
        MemoryOutputStream imageStream;

        if (link.startsWith ("data:"))
        {
            const auto indexOfComma = link.indexOf (",");
            auto format = link.substring (5, indexOfComma).trim();
            auto indexOfSemi = format.indexOf (";");

            if (format.substring (indexOfSemi + 1).trim().equalsIgnoreCase ("base64"))
            {
                auto mime = format.substring (0, indexOfSemi).trim();

                if (mime.equalsIgnoreCase ("image/png") || mime.equalsIgnoreCase ("image/jpeg"))
                {
                    auto base64text = link.substring (indexOfComma + 1).removeCharacters ("\t\n\r ");

                    if (Base64::convertFromBase64 (imageStream, base64text))
                        inputStream.reset (new MemoryInputStream (imageStream.getData(), imageStream.getDataSize(), false));
                }
            }
        }
        else
        {
            auto linkedFile = originalFile.getSiblingFile (link);

            if (linkedFile.existsAsFile())
                inputStream = linkedFile.createInputStream();
        }

        if (inputStream != nullptr)
        {
            auto image = ImageFileFormat::loadFrom (*inputStream);

            if (image.isValid())
            {
                auto* di = new DrawableImage();

                setCommonAttributes (*di, xml);

                Rectangle<float> imageBounds (parseSafeFloat (xml->getStringAttribute ("x")),
                                              parseSafeFloat (xml->getStringAttribute ("y")),
                                              parseSafeFloat (xml->getStringAttribute ("width",  String (image.getWidth()))),
                                              parseSafeFloat (xml->getStringAttribute ("height", String (image.getHeight()))));

                di->setImage (image.rescaled ((int) imageBounds.getWidth(),
                                              (int) imageBounds.getHeight()));

                const auto placement = RectanglePlacement (parsePlacementFlags (xml->getStringAttribute ("preserveAspectRatio").trim()));

                const auto drawableTransform = placement.getTransformToFit (di->getDrawableBounds(), imageBounds)
                                                        .followedBy (transform);

                const auto finalTransform = additionalTransform != nullptr
                    ? drawableTransform.followedBy (*additionalTransform)
                    : drawableTransform;

                di->setDrawableTransform (finalTransform);

                return di;
            }
        }

        return nullptr;
    }

    //==============================================================================
    void addTransform (const XmlPath& xml)
    {
        transform = parseTransform (xml->getStringAttribute ("transform"))
                        .followedBy (transform);
    }

    //==============================================================================
    enum class Axis { x, y };

    bool parseCoord (String::CharPointerType& s, float& value, bool allowUnits, Axis axis) const
    {
        String number;

        if (! parseNextNumber (s, number, allowUnits))
        {
            value = 0;
            return false;
        }

        value = getCoordLength (number, axis == Axis::x ? viewBoxW : viewBoxH);
        return true;
    }

    bool parseCoords (String::CharPointerType& s, Point<float>& p, bool allowUnits) const
    {
        return parseCoord (s, p.x, allowUnits, Axis::x)
            && parseCoord (s, p.y, allowUnits, Axis::y);
    }

    bool parseCoordsOrSkip (String::CharPointerType& s, Point<float>& p, bool allowUnits) const
    {
        if (parseCoords (s, p, allowUnits))
            return true;

        if (! s.isEmpty()) ++s;
        return false;
    }

    float getCoordLength (const String& s, const float sizeForProportions) const noexcept
    {
        auto n = parseSafeFloat (s);
        auto len = s.length();

        if (len > 2)
        {
            auto dpi = 96.0f;

            auto n1 = s[len - 2];
            auto n2 = s[len - 1];

            if (n1 == 'i' && n2 == 'n')         n *= dpi;
            else if (n1 == 'm' && n2 == 'm')    n *= dpi / 25.4f;
            else if (n1 == 'c' && n2 == 'm')    n *= dpi / 2.54f;
            else if (n1 == 'p' && n2 == 'c')    n *= 15.0f;
            else if (n2 == '%')                 n *= 0.01f * sizeForProportions;
        }

        return n;
    }

    float getCoordLength (const XmlPath& xml, const char* attName, const float sizeForProportions) const noexcept
    {
        return getCoordLength (xml->getStringAttribute (attName), sizeForProportions);
    }

    Array<float> getCoordList (const XmlElement& xml, Axis axis) const
    {
        const String attributeName { axis == Axis::x ? "x" : "y" };

        if (! xml.hasAttribute (attributeName))
            return {};

        return getCoordList (xml.getStringAttribute (attributeName), true, axis);
    }

    Array<float> getCoordList (const String& list, bool allowUnits, Axis axis) const
    {
        auto text = list.getCharPointer();
        float value;
        Array<float> coords;

        while (parseCoord (text, value, allowUnits, axis))
            coords.add (value);

        return coords;
    }

    static float parseSafeFloat (const String& s)
    {
        auto n = s.getFloatValue();
        return (std::isnan (n) || std::isinf (n)) ? 0.0f : n;
    }

    //==============================================================================
    void parseCSSStyle (const XmlPath& xml)
    {
        cssStyleText = xml->getAllSubText() + "\n" + cssStyleText;
    }

    void parseDefs (const XmlPath& xml)
    {
        if (auto* style = xml->getChildByName ("style"))
            parseCSSStyle (xml.getChild (style));
    }

    static String::CharPointerType findStyleItem (String::CharPointerType source, String::CharPointerType name)
    {
        auto nameLength = (int) name.length();

        while (! source.isEmpty())
        {
            if (source.getAndAdvance() == '.'
                 && CharacterFunctions::compareIgnoreCaseUpTo (source, name, nameLength) == 0)
            {
                auto endOfName = (source + nameLength).findEndOfWhitespace();

                if (*endOfName == '{')
                    return endOfName;

                if (*endOfName == ',')
                    return CharacterFunctions::find (endOfName, (juce_wchar) '{');
            }
        }

        return source;
    }

    String getStyleAttribute (const XmlPath& xml, StringRef attributeName, const String& defaultValue = String()) const
    {
        if (xml->hasAttribute (attributeName))
            return xml->getStringAttribute (attributeName, defaultValue);

        auto styleAtt = xml->getStringAttribute ("style");

        if (styleAtt.isNotEmpty())
        {
            auto value = getAttributeFromStyleList (styleAtt, attributeName, {});

            if (value.isNotEmpty())
                return value;
        }
        else if (xml->hasAttribute ("class"))
        {
            for (auto i = cssStyleText.getCharPointer();;)
            {
                auto openBrace = findStyleItem (i, xml->getStringAttribute ("class").getCharPointer());

                if (openBrace.isEmpty())
                    break;

                auto closeBrace = CharacterFunctions::find (openBrace, (juce_wchar) '}');

                if (closeBrace.isEmpty())
                    break;

                auto value = getAttributeFromStyleList (String (openBrace + 1, closeBrace),
                                                        attributeName, defaultValue);
                if (value.isNotEmpty())
                    return value;

                i = closeBrace + 1;
            }
        }

        if (xml.parent != nullptr)
            return getStyleAttribute (*xml.parent, attributeName, defaultValue);

        return defaultValue;
    }

    String getInheritedAttribute (const XmlPath& xml, StringRef attributeName) const
    {
        if (xml->hasAttribute (attributeName))
            return xml->getStringAttribute (attributeName);

        if (xml.parent != nullptr)
            return getInheritedAttribute (*xml.parent, attributeName);

        return {};
    }

    static int parsePlacementFlags (const String& align) noexcept
    {
        if (align.isEmpty())
            return 0;

        if (isNone (align))
            return RectanglePlacement::stretchToFit;

        return (align.containsIgnoreCase ("slice") ? RectanglePlacement::fillDestination : 0)
             | (align.containsIgnoreCase ("xMin")  ? RectanglePlacement::xLeft
                                                   : (align.containsIgnoreCase ("xMax") ? RectanglePlacement::xRight
                                                                                        : RectanglePlacement::xMid))
             | (align.containsIgnoreCase ("yMin")  ? RectanglePlacement::yTop
                                                   : (align.containsIgnoreCase ("yMax") ? RectanglePlacement::yBottom
                                                                                        : RectanglePlacement::yMid));
    }

    //==============================================================================
    static bool isIdentifierChar (juce_wchar c)
    {
        return CharacterFunctions::isLetter (c) || c == '-';
    }

    static String getAttributeFromStyleList (const String& list, StringRef attributeName, const String& defaultValue)
    {
        int i = 0;

        for (;;)
        {
            i = list.indexOf (i, attributeName);

            if (i < 0)
                break;

            if ((i == 0 || (i > 0 && ! isIdentifierChar (list [i - 1])))
                 && ! isIdentifierChar (list [i + attributeName.length()]))
            {
                i = list.indexOfChar (i, ':');

                if (i < 0)
                    break;

                int end = list.indexOfChar (i, ';');

                if (end < 0)
                    end = 0x7ffff;

                return list.substring (i + 1, end).trim();
            }

            ++i;
        }

        return defaultValue;
    }

    //==============================================================================
    static bool isStartOfNumber (juce_wchar c) noexcept
    {
        return CharacterFunctions::isDigit (c) || c == '-' || c == '+';
    }

    static bool parseNextNumber (String::CharPointerType& text, String& value, bool allowUnits)
    {
        auto s = text;

        while (s.isWhitespace() || *s == ',')
            ++s;

        auto start = s;

        if (isStartOfNumber (*s))
            ++s;

        while (s.isDigit())
            ++s;

        if (*s == '.')
        {
            ++s;

            while (s.isDigit())
                ++s;
        }

        if ((*s == 'e' || *s == 'E') && isStartOfNumber (s[1]))
        {
            s += 2;

            while (s.isDigit())
                ++s;
        }

        if (allowUnits)
            while (s.isLetter())
                ++s;

        if (s == start)
        {
            text = s;
            return false;
        }

        value = String (start, s);

        while (s.isWhitespace() || *s == ',')
            ++s;

        text = s;
        return true;
    }

    static bool parseNextFlag (String::CharPointerType& text, bool& value)
    {
        while (text.isWhitespace() || *text == ',')
            ++text;

        if (*text != '0' && *text != '1')
            return false;

        value = *(text++) != '0';

        while (text.isWhitespace() || *text == ',')
             ++text;

        return true;
    }

    //==============================================================================
    Colour parseColour (const XmlPath& xml, StringRef attributeName, const Colour defaultColour) const
    {
        auto text = getStyleAttribute (xml, attributeName);

        if (text.startsWithChar ('#'))
        {
            uint32 hex[8] = { 0 };
            hex[6] = hex[7] = 15;

            int numChars = 0;
            auto s = text.getCharPointer();

            while (numChars < 8)
            {
                auto hexValue = CharacterFunctions::getHexDigitValue (*++s);

                if (hexValue >= 0)
                    hex[numChars++] = (uint32) hexValue;
                else
                    break;
            }

            if (numChars <= 3)
                return Colour ((uint8) (hex[0] * 0x11),
                               (uint8) (hex[1] * 0x11),
                               (uint8) (hex[2] * 0x11));

            return Colour ((uint8) ((hex[0] << 4) + hex[1]),
                           (uint8) ((hex[2] << 4) + hex[3]),
                           (uint8) ((hex[4] << 4) + hex[5]),
                           (uint8) ((hex[6] << 4) + hex[7]));
        }

        if (text.startsWith ("rgb") || text.startsWith ("hsl"))
        {
            auto tokens = [&text]
            {
                auto openBracket = text.indexOfChar ('(');
                auto closeBracket = text.indexOfChar (openBracket, ')');

                StringArray arr;

                if (openBracket >= 3 && closeBracket > openBracket)
                {
                    arr.addTokens (text.substring (openBracket + 1, closeBracket), ",", "");
                    arr.trim();
                    arr.removeEmptyStrings();
                }

                return arr;
            }();

            auto alpha = [&tokens, &text]
            {
                if ((text.startsWith ("rgba") || text.startsWith ("hsla")) && tokens.size() == 4)
                    return parseSafeFloat (tokens[3]);

                return 1.0f;
            }();

            if (text.startsWith ("hsl"))
                return Colour::fromHSL (parseSafeFloat (tokens[0]) / 360.0f,
                                        parseSafeFloat (tokens[1]) / 100.0f,
                                        parseSafeFloat (tokens[2]) / 100.0f,
                                        alpha);

            if (tokens[0].containsChar ('%'))
                return Colour ((uint8) roundToInt (2.55f * parseSafeFloat (tokens[0])),
                               (uint8) roundToInt (2.55f * parseSafeFloat (tokens[1])),
                               (uint8) roundToInt (2.55f * parseSafeFloat (tokens[2])),
                               alpha);

            return Colour ((uint8) tokens[0].getIntValue(),
                           (uint8) tokens[1].getIntValue(),
                           (uint8) tokens[2].getIntValue(),
                           alpha);
        }

        if (text == "inherit")
        {
            for (const XmlPath* p = xml.parent; p != nullptr; p = p->parent)
                if (getStyleAttribute (*p, attributeName).isNotEmpty())
                    return parseColour (*p, attributeName, defaultColour);
        }

        return Colours::findColourForName (text, defaultColour);
    }

    static AffineTransform parseTransform (String t)
    {
        AffineTransform result;

        while (t.isNotEmpty())
        {
            StringArray tokens;
            tokens.addTokens (t.fromFirstOccurrenceOf ("(", false, false)
                               .upToFirstOccurrenceOf (")", false, false),
                              ", ", "");

            tokens.removeEmptyStrings (true);

            float numbers[6];

            for (int i = 0; i < numElementsInArray (numbers); ++i)
                numbers[i] = parseSafeFloat (tokens[i]);

            AffineTransform trans;

            if (t.startsWithIgnoreCase ("matrix"))
            {
                trans = AffineTransform (numbers[0], numbers[2], numbers[4],
                                         numbers[1], numbers[3], numbers[5]);
            }
            else if (t.startsWithIgnoreCase ("translate"))
            {
                trans = AffineTransform::translation (numbers[0], numbers[1]);
            }
            else if (t.startsWithIgnoreCase ("scale"))
            {
                trans = AffineTransform::scale (numbers[0], numbers[tokens.size() > 1 ? 1 : 0]);
            }
            else if (t.startsWithIgnoreCase ("rotate"))
            {
                trans = AffineTransform::rotation (degreesToRadians (numbers[0]), numbers[1], numbers[2]);
            }
            else if (t.startsWithIgnoreCase ("skewX"))
            {
                trans = AffineTransform::shear (std::tan (degreesToRadians (numbers[0])), 0.0f);
            }
            else if (t.startsWithIgnoreCase ("skewY"))
            {
                trans = AffineTransform::shear (0.0f, std::tan (degreesToRadians (numbers[0])));
            }

            result = trans.followedBy (result);
            t = t.fromFirstOccurrenceOf (")", false, false).trimStart();
        }

        return result;
    }

    static void endpointToCentreParameters (double x1, double y1,
                                            double x2, double y2,
                                            double angle,
                                            bool largeArc, bool sweep,
                                            double& rx, double& ry,
                                            double& centreX, double& centreY,
                                            double& startAngle, double& deltaAngle) noexcept
    {
        const double midX = (x1 - x2) * 0.5;
        const double midY = (y1 - y2) * 0.5;

        const double cosAngle = std::cos (angle);
        const double sinAngle = std::sin (angle);
        const double xp = cosAngle * midX + sinAngle * midY;
        const double yp = cosAngle * midY - sinAngle * midX;
        const double xp2 = xp * xp;
        const double yp2 = yp * yp;

        double rx2 = rx * rx;
        double ry2 = ry * ry;

        const double s = (xp2 / rx2) + (yp2 / ry2);
        double c;

        if (s <= 1.0)
        {
            c = std::sqrt (jmax (0.0, ((rx2 * ry2) - (rx2 * yp2) - (ry2 * xp2))
                                         / (( rx2 * yp2) + (ry2 * xp2))));

            if (largeArc == sweep)
                c = -c;
        }
        else
        {
            const double s2 = std::sqrt (s);
            rx *= s2;
            ry *= s2;
            c = 0;
        }

        const double cpx = ((rx * yp) / ry) * c;
        const double cpy = ((-ry * xp) / rx) * c;

        centreX = ((x1 + x2) * 0.5) + (cosAngle * cpx) - (sinAngle * cpy);
        centreY = ((y1 + y2) * 0.5) + (sinAngle * cpx) + (cosAngle * cpy);

        const double ux = (xp - cpx) / rx;
        const double uy = (yp - cpy) / ry;
        const double vx = (-xp - cpx) / rx;
        const double vy = (-yp - cpy) / ry;

        const double length = juce_hypot (ux, uy);

        startAngle = acos (jlimit (-1.0, 1.0, ux / length));

        if (uy < 0)
            startAngle = -startAngle;

        startAngle += MathConstants<double>::halfPi;

        deltaAngle = acos (jlimit (-1.0, 1.0, ((ux * vx) + (uy * vy))
                                                / (length * juce_hypot (vx, vy))));

        if ((ux * vy) - (uy * vx) < 0)
            deltaAngle = -deltaAngle;

        if (sweep)
        {
            if (deltaAngle < 0)
                deltaAngle += MathConstants<double>::twoPi;
        }
        else
        {
            if (deltaAngle > 0)
                deltaAngle -= MathConstants<double>::twoPi;
        }

        deltaAngle = fmod (deltaAngle, MathConstants<double>::twoPi);
    }

    SVGState (const SVGState&) = default;
    SVGState& operator= (const SVGState&) = delete;
};

//==============================================================================
std::unique_ptr<Drawable> Drawable::createFromSVG (const XmlElement& svgDocument)
{
    if (! svgDocument.hasTagNameIgnoringNamespace ("svg"))
        return {};

    SVGState state (&svgDocument);
    return std::unique_ptr<Drawable> (state.parseSVGElement (SVGState::XmlPath (&svgDocument, {})));
}

std::unique_ptr<Drawable> Drawable::createFromSVGFile (const File& svgFile)
{
    if (auto xml = parseXMLIfTagMatches (svgFile, "svg"))
        return createFromSVG (*xml);

    return {};
}

Path Drawable::parseSVGPath (const String& svgPath)
{
    const auto svgString = "<svg xmlns=\"http://www.w3.org/2000/svg\"><path d=" + svgPath.quoted() + " /></svg>";
    const auto composite = createFromSVGString (svgString);

    if (composite->getNumChildren() != 1)
    {
        jassertfalse;
        return {};
    }

    auto* p = dynamic_cast<const DrawablePath*> (&composite->getChild (0));

    if (p == nullptr)
        return {};

    return p->getPath();
}

struct LunaSvgConverters
{
    LunaSvgConverters() = delete;

    static Path toJucePath (const lunasvg::Path& p,
                            const lunasvg::FillRule& fillRule = lunasvg::FillRule::NonZero)
    {
        juce::Path output;

        for (auto it = lunasvg::PathIterator { p }; ! it.isDone(); it.next())
        {
            std::array<lunasvg::Point, 3> points;

            switch (it.currentSegment (points))
            {
                case lunasvg::PathCommand::MoveTo:
                    output.startNewSubPath (points[0].x, points[0].y);
                    break;

                case lunasvg::PathCommand::LineTo:
                    output.lineTo (points[0].x, points[0].y);
                    break;

                case lunasvg::PathCommand::CubicTo:
                    output.cubicTo (
                        points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
                    break;

                case lunasvg::PathCommand::Close:
                    output.closeSubPath();
                    break;
            }
        }

        output.setUsingNonZeroWinding (fillRule == lunasvg::FillRule::NonZero);
        return output;
    }

    static Rectangle<float> toJuceRectangle (const lunasvg::Rect& r)
    {
        return { r.x, r.y, r.w, r.h };
    }

    static PathStrokeType::JointStyle toJuceJointStyle (const lunasvg::LineJoin& lineCap)
    {
        switch (lineCap)
        {
            case lunasvg::LineJoin::Miter:
                return PathStrokeType::mitered;

            case lunasvg::LineJoin::Round:
                return PathStrokeType::curved;

            case lunasvg::LineJoin::Bevel:
                return PathStrokeType::beveled;
        }

        jassertfalse;
        return PathStrokeType::beveled;
    }

    static PathStrokeType::EndCapStyle toJuceEndCapStyle (const lunasvg::LineCap& lineCap)
    {
        switch (lineCap)
        {
            case lunasvg::LineCap::Butt:
                return PathStrokeType::butt;

            case lunasvg::LineCap::Round:
                return PathStrokeType::rounded;

            case lunasvg::LineCap::Square:
                return PathStrokeType::square;
        }

        jassertfalse;
        return PathStrokeType::square;
    }

    static AffineTransform toJuceTransform (const lunasvg::Transform& transform)
    {
        const auto& m = transform.matrix();
        return AffineTransform { m.a, m.c, m.e, m.b, m.d, m.f };
    }

    static Colour toJuceColour (const lunasvg::Color& colour)
    {
        return Colour::fromRGBA (colour.red(), colour.green(), colour.blue(), colour.alpha());
    }

    static Colour toJuceColour (const juce_plutovg_color_t& colour)
    {
        return Colour::fromFloatRGBA (colour.r, colour.g, colour.b, colour.a);
    }

    static ColourGradient::SpreadMethod toJuceSpreadMethod (const lunasvg::SpreadMethod& spread)
    {
        switch (spread)
        {
            case lunasvg::SpreadMethod::Pad:
                return ColourGradient::SpreadMethod::pad;

            case lunasvg::SpreadMethod::Reflect:
                return ColourGradient::SpreadMethod::reflect;

            case lunasvg::SpreadMethod::Repeat:
                return ColourGradient::SpreadMethod::repeat;
        }

        jassertfalse;
        return ColourGradient::SpreadMethod::pad;
    }

    static Image toJuceImage (juce_plutovg_surface* s)
    {
        if (s == nullptr || s->data == nullptr || s->width <= 0 || s->height <= 0)
            return {};

        Image image { Image::ARGB, s->width, s->height, false };
        Image::BitmapData destData { image, Image::BitmapData::writeOnly };

        for (int y = 0; y < s->height; ++y)
        {
            const auto* src = s->data + (size_t) (y * s->stride);
            auto* dest = destData.getLinePointer (y);
            memcpy (dest, src, (size_t) s->width * 4);
        }

        return image;
    }

    static BlendMode toJuceBlendMode (const lunasvg::BlendMode& blendMode)
    {
        switch (blendMode)
        {
            case lunasvg::BlendMode::Src:       return BlendMode::source;
            case lunasvg::BlendMode::Src_Over:  return BlendMode::sourceOver;
            case lunasvg::BlendMode::Dst_In:    return BlendMode::destinationIn;
            case lunasvg::BlendMode::Dst_Out:   return BlendMode::destinationOut;
        }

        jassertfalse;
        return BlendMode::source;
    }

    static Rectangle<int> toJuceIntRectangle (const lunasvg::Rect& r)
    {
        return Rectangle<float> { r.x, r.y, r.w, r.h }.toNearestInt();
    }

    static lunasvg::Rect fromJuceRectangle (const Rectangle<float>& r)
    {
        return lunasvg::Rect { r.getX(), r.getY(), r.getWidth(), r.getHeight() };
    }
};

namespace
{

class DrawableCanvas : public lunasvg::Canvas
{
private:
    struct State
    {
        FillType fillType;
        std::shared_ptr<Drawable> clipPath;
    };

protected:
    std::shared_ptr<Canvas> createFromBounds (int x, int y, int width, int height) override
    {
        auto canvas = std::make_shared<DrawableCanvas> (&document);
        canvas->setContentArea (Rectangle<int> (x, y, width, height).toFloat());
        return canvas;
    }

    std::shared_ptr<Canvas> createFromBitmap (const lunasvg::Bitmap&) override
    {
        jassertfalse;
        return {};
    }

public:
    explicit DrawableCanvas (const lunasvg::Document* documentIn)
        : document (*documentIn)
    {
    }

    void setColor (const lunasvg::Color& color) override
    {
        getState().fillType = FillType { LunaSvgConverters::toJuceColour (color) };
    }

    void setColor (float r, float g, float b, float a) override
    {
        getState().fillType = Colour::fromFloatRGBA (r, g, b, a);
    }

    void setLinearGradient (float x1,
                            float y1,
                            float x2,
                            float y2,
                            lunasvg::SpreadMethod spread,
                            const lunasvg::GradientStops& stops,
                            const lunasvg::Transform& transform) override
    {
        ColourGradient gradient;
        gradient.point1 = { x1, y1 };
        gradient.point2 = { x2, y2 };
        gradient.spreadMethod = LunaSvgConverters::toJuceSpreadMethod (spread);

        for (const auto& stop : stops)
            gradient.addColour (stop.offset, LunaSvgConverters::toJuceColour (stop.color));

        const auto gradientTransform = LunaSvgConverters::toJuceTransform (transform);
        getState().fillType = FillType { gradient }.transformed (gradientTransform);
    }

    void setRadialGradient (float cx,
                            float cy,
                            float r,
                            float fx,
                            float fy,
                            lunasvg::SpreadMethod spread,
                            const lunasvg::GradientStops& stops,
                            const lunasvg::Transform& transform) override
    {
        ColourGradient gradient;
        gradient.isRadial = true;

        for (const auto& stop : stops)
            gradient.addColour (stop.offset, LunaSvgConverters::toJuceColour (stop.color));

        const auto gradientTransform = LunaSvgConverters::toJuceTransform (transform);
        const Point<float> endCentre { cx, cy };
        const Point<float> startCentre { fx, fy };

        gradient.point1 = startCentre;
        gradient.point2 = endCentre;
        gradient.endRadius = r;
        gradient.spreadMethod = LunaSvgConverters::toJuceSpreadMethod (spread);

        getState().fillType = FillType { gradient }.transformed (gradientTransform);
    }

    void setTexture (const Canvas& source,
                     lunasvg::TextureType type,
                     float opacity,
                     const lunasvg::Transform& transform) override
    {
        jassertquiet (type == lunasvg::TextureType::Tiled);
        const DrawableCanvas* drawableCanvas = dynamic_cast<const DrawableCanvas*> (&source);

        if (drawableCanvas == nullptr)
        {
            jassertfalse;
            return;
        }

        auto image = drawableCanvas->getJuceSurface();
        auto fill = FillType { std::move (image), LunaSvgConverters::toJuceTransform (transform) };
        fill.setOpacity (opacity);
        getState().fillType = std::move (fill);
    }

    static void applyClipPath (const Drawable* clip, Drawable& drawable)
    {
        if (clip == nullptr)
            return;

        auto clipForDrawable = clip->createCopy();
        auto t = clip->getDrawableTransform();
        clipForDrawable->setDrawableTransform (t.followedBy (drawable.getDrawableTransform().inverted()));
        drawable.setClipPath (std::move (clipForDrawable));
    }

    void fillPath (const lunasvg::Path& path,
                   lunasvg::FillRule fillRule,
                   const lunasvg::Transform& transform) override
    {
        auto p = std::make_unique<DrawablePath>();
        p->setName (document.getElementIdBeingRendered());
        p->setFill (getState().fillType);
        p->setPath (LunaSvgConverters::toJucePath (path, fillRule));
        p->setDrawableTransform (LunaSvgConverters::toJuceTransform (transform));
        applyClipPath (getState().clipPath.get(), *p);
        lastPath = p.get();
        composite->addChild (std::move (p));
    }

    void strokePath (const lunasvg::Path& path,
                     const lunasvg::StrokeData& strokeData,
                     const lunasvg::Transform& transform) override
    {
        const auto configureStrokeAndResetLastPath = [&] (DrawablePath& drawablePath)
        {
            drawablePath.setStrokeFill (getState().fillType);

            PathStrokeType strokeType { strokeData.lineWidth(),
                                        LunaSvgConverters::toJuceJointStyle (strokeData.lineJoin()),
                                        LunaSvgConverters::toJuceEndCapStyle (strokeData.lineCap()) };

            strokeType.setMiterLimit (strokeData.miterLimit());
            drawablePath.setStrokeType (strokeType);

            drawablePath.setDashLengths (strokeData.dashArray());
            drawablePath.setDashOffset (strokeData.dashOffset());

            lastPath = nullptr;
        };

        const auto jucePath = LunaSvgConverters::toJucePath (path);
        const auto juceTransform = LunaSvgConverters::toJuceTransform (transform);

        if (lastPath != nullptr
            && lastPath->getPath() == jucePath
            && lastPath->getDrawableTransform() == juceTransform)
        {
            configureStrokeAndResetLastPath (*lastPath);
            return;
        }

        auto p = std::make_unique<DrawablePath>();
        p->setName (document.getElementIdBeingRendered());
        p->setFill (Colour::fromRGBA (0, 0, 0, 0));
        configureStrokeAndResetLastPath (*p);
        p->setPath (jucePath);
        p->setDrawableTransform (juceTransform);
        applyClipPath (getState().clipPath.get(), *p);
        composite->addChild (std::move (p));
    }

    class LunaSvgTextParameters
    {
    public:
        LunaSvgTextParameters (const std::u32string_view& textIn,
                               const lunasvg::Font& fontIn,
                               const lunasvg::Point& originIn,
                               const lunasvg::Transform& transformIn)
            : text { toJuceString (textIn) },
              font { fontIn.getFont() },
              origin { originIn.x, originIn.y },
              transform { LunaSvgConverters::toJuceTransform (transformIn) }
        {
        }

        std::unique_ptr<DrawableText> createDrawableText()
        {
            auto t = std::make_unique<DrawableText>();
            t->setPreserveWhitespace (true);
            t->setFont (font, true);
            t->setDrawableTransform (transform);
            t->setText (text);

            const Rectangle<float> bounds { origin.x,
                                            origin.y - font.getAscent(),
                                            GlyphArrangement::getStringWidth (font, text),
                                            font.getHeight() };

            t->setBoundingBox (bounds);
            drawableText = t.get();

            return t;
        }

        void setStrokeOptionsOnLastObject (const StrokeOptions& attrs)
        {
            if (drawableText == nullptr)
                return;

            drawableText->setStrokeOptions (attrs);
        }

        void setFillOnLastObject (const FillType& fill)
        {
            if (drawableText == nullptr)
                return;

            drawableText->setFill (fill);
        }

        bool operator== (const LunaSvgTextParameters& other) const noexcept
        {
            return text == other.text
                && font == other.font
                && origin == other.origin
                && transform == other.transform;
        }

        bool operator!= (const LunaSvgTextParameters& other) const noexcept
        {
            return ! operator== (other);
        }

    private:
        static String toJuceString (const std::u32string_view& view)
        {
            return juce::String { juce::CharPointer_UTF32 (reinterpret_cast<const CharPointer_UTF32::CharType*> (view.data())),
                                  view.length() };
        }

        String text;
        Font font;
        Point<float> origin;
        AffineTransform transform;
        DrawableText* drawableText = nullptr;
    };

    void fillText (const std::u32string_view& text,
                   const lunasvg::Font& font,
                   const lunasvg::Point& origin,
                   const lunasvg::Transform& transform) override
    {
        lastTextParams = LunaSvgTextParameters { text, font, origin, transform };
        auto t = lastTextParams->createDrawableText();
        t->setName (document.getElementIdBeingRendered());
        t->setFill (getState().fillType);
        applyClipPath (getState().clipPath.get(), *t);
        composite->addChild (std::move (t));
    }

    void strokeText (const std::u32string_view& text,
                     float strokeWidth,
                     const lunasvg::Font& font,
                     const lunasvg::Point& origin,
                     const lunasvg::Transform& transform) override
    {
        const auto attrs = StrokeOptions{}.withWidth (strokeWidth).withColour (getState().fillType.colour);
        LunaSvgTextParameters textParams { text, font, origin, transform };

        if (auto lastParams = std::exchange (lastTextParams, {}); lastParams.has_value() && *lastParams == textParams)
        {
            lastParams->setStrokeOptionsOnLastObject (attrs);
            return;
        }

        auto t = textParams.createDrawableText();
        t->setName (document.getElementIdBeingRendered());
        t->setFill (FillType { Colours::transparentBlack });
        t->setStrokeOptions (attrs);
        applyClipPath (getState().clipPath.get(), *t);
        composite->addChild (std::move (t));
    }

    void clipPath (const Path& path,
                   const lunasvg::Transform& transform)
    {
        auto clip = std::make_unique<DrawablePath>();
        clip->setPath (path);
        clip->setDrawableTransform (LunaSvgConverters::toJuceTransform (transform));
        getState().clipPath = std::move (clip);
    }

    void clipPath (const lunasvg::Path& path,
                   lunasvg::FillRule clipRule,
                   const lunasvg::Transform& transform) override
    {
        clipPath (LunaSvgConverters::toJucePath (path, clipRule), transform);
    }

    void clipRect (const lunasvg::Rect& rect,
                   lunasvg::FillRule clipRule,
                   const lunasvg::Transform& transform) override
    {
        auto r = LunaSvgConverters::toJuceRectangle (rect);
        Path p;
        p.addRectangle (r);
        p.setUsingNonZeroWinding (clipRule == lunasvg::FillRule::NonZero);
        clipPath (p, transform);
    }

    void drawImage (const lunasvg::Bitmap& image,
                    const lunasvg::Rect& dstRect,
                    const lunasvg::Rect& srcRect,
                    const lunasvg::Transform& transform) override
    {
        auto i = std::make_unique<DrawableImage> (LunaSvgConverters::toJuceImage (image.surface()),
                                                  LunaSvgConverters::toJuceIntRectangle (dstRect),
                                                  LunaSvgConverters::toJuceIntRectangle (srcRect));
        i->setName (document.getElementIdBeingRendered());

        i->setDrawableTransform (LunaSvgConverters::toJuceTransform (transform));
        applyClipPath (getState().clipPath.get(), *i);
        composite->addChild (std::move (i));
    }

    void blendCanvas (const Canvas& canvas, lunasvg::BlendMode blendMode, float opacity) override
    {
        const DrawableCanvas* drawableCanvas = dynamic_cast<const DrawableCanvas*> (&canvas);

        if (drawableCanvas == nullptr)
        {
            // If this happened, we didn't replace all canvas objects inside lunasvg with DrawableCanvas
            jassertfalse;
            return;
        }

        std::unique_ptr<DrawableComposite> other { static_cast<DrawableComposite*> (drawableCanvas->composite->createCopy().release()) };
        other->setBlendMode (LunaSvgConverters::toJuceBlendMode (blendMode));
        other->setOpacity (opacity);
        composite->addChild (std::move (other));
    }

    void save() override
    {
        saveState();
    }

    void restore() override
    {
        restoreState();
    }

    void convertToLuminanceMask() override
    {
        composite->setLuminanceMask (true);
    }

    int x() const override
    {
        return getContentArea().getSmallestIntegerContainer().getX();
    }

    int y() const override
    {
        return getContentArea().getSmallestIntegerContainer().getY();
    }

    int width() const override
    {
        return getContentArea().getSmallestIntegerContainer().getWidth();
    }

    int height() const override
    {
        return getContentArea().getSmallestIntegerContainer().getHeight();
    }

    lunasvg::Rect extents() const override
    {
        return LunaSvgConverters::fromJuceRectangle (getContentArea().getSmallestIntegerContainer().toFloat());
    }

    Image getJuceSurface() const
    {
        const auto extents = getContentArea().getSmallestIntegerContainer();
        Image image { Image::ARGB, extents.getWidth(), extents.getHeight(), true };
        Graphics g (image);
        g.addTransform (AffineTransform::translation ((float) -extents.getX(), (float) -extents.getY()));
        composite->draw (g, 1.0f);
        return image;
    }

    juce_plutovg_surface_t* surface() const override
    {
        // We could implement this by converting the result of getJuceSurface(), but if every angle
        // is covered by our code, this shouldn't be called, and our code can call getJuceSurface()
        // directly.
        jassertfalse;
        return nullptr;
    }

    juce_plutovg_canvas_t* canvas() const override
    {
        // Not implemented
        jassertfalse;
        return nullptr;
    }

    std::unique_ptr<DrawableComposite> release()
    {
        return std::move (composite);
    }

    State& getState()
    {
        if (state.empty())
            state.push (State());

        return state.top();
    }

    void saveState()
    {
        state.push (getState());
    }

    void restoreState()
    {
        if (state.size() > 1)
            state.pop();
    }

    void setContentArea (const Rectangle<float>& area)
    {
        composite->setContentArea (area);
    }

    Rectangle<float> getContentArea() const
    {
        return composite->getContentArea();
    }

    const lunasvg::Document& document;
    std::stack<State> state;
    std::unique_ptr<DrawableComposite> composite = std::make_unique<DrawableComposite>();
    DrawablePath* lastPath = nullptr;
    std::optional<LunaSvgTextParameters> lastTextParams;
};

}

std::unique_ptr<DrawableComposite> Drawable::createFromSVGString (const String& svgString)
{
    const auto document = lunasvg::Document::loadFromData (svgString.toStdString());

    if (document == nullptr)
        return {};

    auto canvas = std::make_shared<DrawableCanvas> (document.get());
    canvas->setContentArea ({ 0.0f, 0.0f, document->width(), document->height() });
    document->renderToCanvas (canvas, lunasvg::Matrix());

    return canvas->release();
}

//==============================================================================
//==============================================================================
#if JUCE_UNIT_TESTS

struct SvgParserTests final : public UnitTest
{
    SvgParserTests() : UnitTest ("SvgParser", UnitTestCategories::graphics)
    {
    }

    void runTest() override
    {
        testCase ("Path parsing regression test", [this]
        {
            const auto juceLogoSvgPathString =
                "m 72.87 84.28 c 49.518 84.341 30.521 65.492 30.4 42.14 30.756 18.936 49.668 "
                "0.312 72.875 0.312 96.082 0.312 114.994 18.936 115.35 42.14 115.229 65.496 "
                "96.226 84.346 72.87 84.28 z m 72.87 5.61 c 52.61 5.538 36.116 21.88 36 42.14 "
                "36.332 62.269 52.744 78.412 72.875 78.412 93.006 78.412 109.418 62.269 109.75 "
                "42.14 109.629 21.879 93.132 5.538 72.87 5.61 z m 77.62 49.59 c 80.16 56.066 "
                "83.079 62.386 86.36 68.52 86.959 69.604 87.99 70.384 89.196 70.666 90.403 70.948 "
                "91.672 70.706 92.69 70 96.165 67.569 99.162 64.518 101.53 61 102.279 59.861 "
                "102.445 58.435 101.975 57.155 101.506 55.876 100.458 54.894 99.15 54.51 92.633 "
                "52.485 86.239 50.084 80 47.32 79.342 47.029 78.573 47.163 78.052 47.66 77.531 "
                "48.157 77.36 48.919 77.62 49.59 z m 81.05 44.27 c 87.597 47.162 94.32 49.637 "
                "101.18 51.68 102.367 52.019 103.642 51.843 104.693 51.194 105.743 50.545 106.472 "
                "49.483 106.7 48.27 107.066 46.247 107.25 44.196 107.25 42.14 107.251 39.883 "
                "107.027 37.632 106.58 35.42 106.306 34.074 105.415 32.934 104.174 32.344 102.933 "
                "31.754 101.487 31.782 100.27 32.42 94.041 35.627 87.642 38.491 81.1 41 80.426 "
                "41.256 79.977 41.897 79.966 42.618 79.955 43.339 80.385 43.993 81.05 44.27 z m "
                "74.47 50.44 c 74.195 49.774 73.545 49.34 72.825 49.34 72.105 49.34 71.455 49.774 "
                "71.18 50.44 68.27 56.903 65.778 63.547 63.72 70.33 63.375 71.521 63.557 72.804 "
                "64.221 73.852 64.884 74.9 65.965 75.613 67.19 75.81 69.068 76.115 70.967 76.269 "
                "72.87 76.27 75.268 76.256 77.657 75.991 80 75.48 81.341 75.219 82.479 74.34 "
                "83.07 73.109 83.661 71.877 83.635 70.439 83 69.23 79.814 63.128 76.967 56.855 "
                "74.47 50.44 z m 71.59 34.12 c 71.855 34.79 72.498 35.234 73.218 35.245 73.939 "
                "35.256 74.595 34.832 74.88 34.17 77.823 27.638 80.336 20.92 82.4 14.06 82.739 "
                "12.88 82.563 11.612 81.915 10.57 81.267 9.527 80.208 8.808 79 8.59 74.674 7.83 "
                "70.244 7.888 65.94 8.76 64.596 9.02 63.455 9.9 62.864 11.135 62.272 12.369 "
                "62.301 13.81 62.94 15.02 66.176 21.221 69.063 27.598 71.59 34.12 z m 46.32 30.3 "
                "c 53.132 32.387 59.811 34.885 66.32 37.78 66.98 38.068 67.748 37.931 68.267 "
                "37.432 68.785 36.933 68.952 36.17 68.69 35.5 66.049 28.709 63.001 22.083 59.56 "
                "15.66 58.959 14.577 57.928 13.8 56.721 13.52 55.515 13.239 54.247 13.483 53.23 "
                "14.19 49.518 16.761 46.351 20.041 43.91 23.84 43.178 24.982 43.026 26.402 43.5 "
                "27.672 43.974 28.943 45.019 29.917 46.32 30.3 z m 68.17 49.18 c 68.453 48.521 "
                "68.311 47.756 67.809 47.243 67.307 46.73 66.545 46.571 65.88 46.84 59.209 49.394 "
                "52.694 52.339 46.37 55.66 45.266 56.244 44.47 57.279 44.19 58.496 43.91 59.713 "
                "44.173 60.992 44.91 62 47.459 65.53 50.656 68.544 54.33 70.88 55.475 71.608 "
                "56.893 71.761 58.167 71.294 59.441 70.828 60.426 69.795 60.83 68.5 62.894 61.921 "
                "65.345 55.47 68.17 49.18 z m 77.79 35.59 c 77.508 36.252 77.652 37.019 78.155 "
                "37.533 78.659 38.047 79.422 38.208 80.09 37.94 86.792 35.368 93.337 32.403 99.69 "
                "29.06 100.773 28.481 101.557 27.466 101.843 26.272 102.13 25.078 101.892 23.818 "
                "101.19 22.81 98.669 19.188 95.474 16.085 91.78 13.67 90.639 12.925 89.216 12.756 "
                "87.932 13.213 86.649 13.67 85.653 14.701 85.24 16 83.151 22.673 80.664 29.215 "
                "77.79 35.59 z m 64.69 40.6 c 58.116 37.689 51.362 35.204 44.47 33.16 43.285 "
                "32.828 42.014 33.012 40.972 33.667 39.931 34.323 39.214 35.388 39 36.6 38.698 "
                "38.431 38.547 40.284 38.55 42.14 38.548 44.629 38.82 47.11 39.36 49.54 39.673 "
                "50.851 40.576 51.944 41.804 52.499 43.032 53.055 44.448 53.011 45.64 52.38 "
                "51.812 49.193 58.155 46.349 64.64 43.86 65.307 43.6 65.751 42.963 65.761 42.247 "
                "65.772 41.531 65.349 40.88 64.69 40.6 z m 20 129.315 c 20 134.315 17.28 137.475 "
                "12.89 137.475 10.52 137.475 8.72 136.475 6.69 133.915 l 6 133.135 -0 138.135 "
                "0.57 138.895 c 3.82 143.255 7.73 145.285 12.88 145.285 21.88 145.285 28.22 "
                "138.715 28.22 129.285 l 28.22 101.185 20 101.185 z m 61.69 126.505 c 61.69 "
                "133.165 57.93 137.505 52.12 137.505 46.31 137.505 42.56 133.195 42.56 126.505 l "
                "42.56 101.185 34.33 101.185 34.33 126.875 c 34.33 137.535 41.73 145.275 51.93 "
                "145.275 61.93 145.275 69.54 137.555 69.93 126.875 l 69.93 101.185 61.69 101.185 "
                "z m 106.83 134.095 c 103.25 136.525 100.65 137.475 97.58 137.475 89.933 136.983 "
                "83.983 130.637 83.983 122.975 83.983 115.313 89.933 108.967 97.58 108.475 100.82 "
                "108.475 103.24 109.355 106.83 111.855 l 107.59 112.385 112.37 106.385 111.62 "
                "105.765 c 107.623 102.453 102.591 100.649 97.4 100.665 89.311 100.494 81.763 "
                "104.711 77.669 111.689 73.574 118.667 73.574 127.313 77.669 134.291 81.763 "
                "141.269 89.311 145.486 97.4 145.315 102.656 145.435 107.774 143.628 111.79 "
                "140.235 l 112.6 139.595 107.6 133.595 z m 145.75 137.285 l 126.69 137.285 126.69 "
                "126.565 144.99 126.565 144.99 118.955 126.69 118.955 126.69 108.795 145.75 "
                "108.795 145.75 101.185 118.47 101.185 118.47 144.715 145.75 144.715 z m 68.015 "
                "83.917 c 60.292 83.015 52.543 79.794 46.449 74.951 37.974 68.215 32.277 58.128 "
                "30.875 47.376 29.303 35.31 33.538 22.7 42.21 13.631 49.154 6.368 58.07 1.902 "
                "68.042 0.695 70.192 0.435 75.566 0.435 77.717 0.695 90.205 2.207 101.181 8.945 "
                "108.154 19.381 116.486 31.852 117.472 47.504 110.759 60.749 108.479 65.249 "
                "106.422 68.108 102.909 71.658 96.123 78.511 87.197 82.838 77.613 83.92 75.586 "
                "84.147 69.969 84.145 68.015 83.917 z m 75.838 78.321 c 84.273 77.906 93.284 "
                "73.643 99.521 67.116 105.497 60.862 108.871 53.393 109.702 44.579 110.334 37.874 "
                "108.356 29.631 104.637 23.471 98.88 13.935 89.397 7.602 78.34 5.906 75.799 5.516 "
                "69.942 5.52 67.38 5.912 53.54 8.034 42.185 17.542 37.81 30.67 35.003 39.096 "
                "35.389 47.937 38.92 56.114 43.797 67.411 53.879 75.524 65.897 77.823 68.033 "
                "78.231 71.997 78.578 73.274 78.468 73.599 78.44 74.754 78.374 75.838 78.321 z";

            const auto juceLogoPathString =
                "m 72.87 84.28 c 122.388 168.621 103.391 149.772 103.27 126.42 134.026 145.356 "
                "152.938 126.732 176.145 126.732 272.227 127.044 291.139 145.668 291.495 168.872 "
                "406.724 234.368 387.721 253.218 364.365 253.152 z m 145.74 89.89 c 198.35 95.428 "
                "181.856 111.77 181.74 132.03 218.072 194.299 234.484 210.442 254.615 210.442 "
                "347.621 288.854 364.033 272.711 364.365 252.582 473.994 274.461 457.497 258.12 "
                "437.235 258.192 z m 223.36 139.48 c 303.52 195.546 306.439 201.866 309.72 208 "
                "396.679 277.604 397.71 278.384 398.916 278.666 489.319 349.614 490.588 349.372 "
                "491.606 348.666 587.771 416.235 590.768 413.184 593.136 409.666 695.415 469.527 "
                "695.581 468.101 695.111 466.821 796.617 522.697 795.569 521.715 794.261 521.331 "
                "886.894 573.816 880.5 571.415 874.261 568.651 953.603 615.68 952.834 615.814 "
                "952.313 616.311 1029.844 664.468 1029.673 665.23 1029.933 665.901 z m 304.41 "
                "183.75 c 392.007 230.912 398.73 233.387 405.59 235.43 507.957 287.449 509.232 "
                "287.273 510.283 286.624 616.026 337.169 616.755 336.107 616.983 334.894 724.049 "
                "381.141 724.233 379.09 724.233 377.034 831.484 416.917 831.26 414.666 830.813 "
                "412.454 937.119 446.528 936.228 445.388 934.987 444.798 1037.92 476.552 1036.474 "
                "476.58 1035.257 477.218 1129.298 512.845 1122.899 515.709 1116.357 518.218 "
                "1196.783 559.474 1196.334 560.115 1196.323 560.836 1276.278 604.175 1276.708 "
                "604.829 1277.373 605.106 z m 378.88 234.19 c 453.075 283.964 452.425 283.53 "
                "451.705 283.53 523.81 332.87 523.16 333.304 522.885 333.97 591.155 390.873 "
                "588.663 397.517 586.605 404.3 649.98 475.821 650.162 477.104 650.826 478.152 "
                "715.71 553.052 716.791 553.765 718.016 553.962 787.084 630.077 788.983 630.231 "
                "790.886 630.232 866.154 706.488 868.543 706.223 870.886 705.712 952.227 780.931 "
                "953.365 780.052 953.956 778.821 1037.617 850.698 1037.591 849.26 1036.956 "
                "848.051 1116.77 911.179 1113.923 904.906 1111.426 898.491 z m 450.47 268.31 c "
                "522.325 303.1 522.968 303.544 523.688 303.555 597.627 338.811 598.283 338.387 "
                "598.568 337.725 676.391 365.363 678.904 358.645 680.968 351.785 763.707 364.665 "
                "763.531 363.397 762.883 362.355 844.15 371.882 843.091 371.163 841.883 370.945 "
                "916.557 378.775 912.127 378.833 907.823 379.705 972.419 388.725 971.278 389.605 "
                "970.687 390.84 1032.959 403.209 1032.988 404.65 1033.627 405.86 1099.803 427.081 "
                "1102.69 433.458 1105.217 439.98 z m 496.79 298.61 c 549.922 330.997 556.601 "
                "333.495 563.11 336.39 630.09 374.458 630.858 374.321 631.377 373.822 700.162 "
                "410.755 700.329 409.992 700.067 409.322 766.116 438.031 763.068 431.405 759.627 "
                "424.982 818.586 439.559 817.555 438.782 816.348 438.502 871.863 451.741 870.595 "
                "451.985 869.578 452.692 919.096 469.453 915.929 472.733 913.488 476.532 956.666 "
                "501.514 956.514 502.934 956.988 504.204 1000.962 533.147 1002.007 534.121 "
                "1003.308 534.504 z m 564.96 347.79 c 633.413 396.311 633.271 395.546 632.769 "
                "395.033 700.076 441.763 699.314 441.604 698.649 441.873 757.858 491.267 751.343 "
                "494.212 745.019 497.533 790.285 553.777 789.489 554.812 789.209 556.029 833.119 "
                "615.742 833.382 617.021 834.119 618.029 881.578 683.559 884.775 686.573 888.449 "
                "688.909 943.924 760.517 945.342 760.67 946.616 760.203 1006.057 831.031 1007.042 "
                "829.998 1007.446 828.703 1070.34 890.624 1072.791 884.173 1075.616 877.883 z m "
                "642.75 383.38 c 720.258 419.632 720.402 420.399 720.905 420.913 799.564 458.96 "
                "800.327 459.121 800.995 458.853 887.787 494.221 894.332 491.256 900.685 487.913 "
                "1001.458 516.394 1002.242 515.379 1002.528 514.185 1104.658 539.263 1104.42 "
                "538.003 1103.718 536.995 1202.387 556.183 1199.192 553.08 1195.498 550.665 "
                "1286.137 563.59 1284.714 563.421 1283.43 563.878 1370.079 577.548 1369.083 "
                "578.579 1368.67 579.878 1451.821 602.551 1449.334 609.093 1446.46 615.468 z m "
                "707.44 423.98 c 765.556 461.669 758.802 459.184 751.91 457.14 795.195 489.968 "
                "793.924 490.152 792.882 490.807 832.813 525.13 832.096 526.195 831.882 527.407 "
                "870.58 565.838 870.429 567.691 870.432 569.547 908.98 614.176 909.252 616.657 "
                "909.792 619.087 949.465 669.938 950.368 671.031 951.596 671.586 994.628 724.641 "
                "996.044 724.597 997.236 723.966 1049.048 773.159 1055.391 770.315 1061.876 "
                "767.826 1127.183 811.426 1127.627 810.789 1127.637 810.073 1193.409 851.604 "
                "1192.986 850.953 1192.327 850.673 z m 727.44 553.295 c 747.44 687.61 744.72 "
                "690.77 740.33 690.77 750.85 828.245 749.05 827.245 747.02 824.685 l 753.02 "
                "957.82 753.02 1095.955 753.59 1234.85 c 757.41 1378.105 761.32 1380.135 766.47 "
                "1380.135 788.35 1525.42 794.69 1518.85 794.69 1509.42 l 822.91 1610.605 842.91 "
                "1711.79 z m 789.13 679.8 c 850.82 812.965 847.06 817.305 841.25 817.305 887.56 "
                "954.81 883.81 950.5 883.81 943.81 l 926.37 1044.995 960.7 1146.18 995.03 "
                "1273.055 c 1029.36 1410.59 1036.76 1418.33 1046.96 1418.33 1108.89 1563.605 "
                "1116.5 1555.885 1116.89 1545.205 l 1186.82 1646.39 1248.51 1747.575 z m 895.96 "
                "813.895 c 999.21 950.42 996.61 951.37 993.54 951.37 1083.473 1088.353 1077.523 "
                "1082.007 1077.523 1074.345 1161.506 1189.658 1167.456 1183.312 1175.103 1182.82 "
                "1275.923 1291.295 1278.343 1292.175 1281.933 1294.675 l 1389.523 1407.06 "
                "1501.893 1513.445 1613.513 1619.21 c 1721.136 1721.663 1716.104 1719.859 "
                "1710.913 1719.875 1800.224 1820.369 1792.676 1824.586 1788.582 1831.564 1862.156 "
                "1950.231 1862.156 1958.877 1866.251 1965.855 1948.014 2107.124 1955.562 2111.341 "
                "1963.651 2111.17 2066.307 2256.605 2071.425 2254.798 2075.441 2251.405 l "
                "2188.041 2391 2295.641 2524.595 z m 1041.71 951.18 l 1168.4 1088.465 1295.09 "
                "1215.03 1440.08 1341.595 1585.07 1460.55 1711.76 1579.505 1838.45 1688.3 1984.2 "
                "1797.095 2129.95 1898.28 2248.42 1999.465 2366.89 2144.18 2512.64 2288.895 z m "
                "1109.725 1035.097 c 1170.017 1118.112 1162.268 1114.891 1156.174 1110.048 "
                "1194.148 1178.263 1188.451 1168.176 1187.049 1157.424 1216.352 1192.734 1220.587 "
                "1180.124 1229.259 1171.055 1278.413 1177.423 1287.329 1172.957 1297.301 1171.75 "
                "1367.493 1172.185 1372.867 1172.185 1375.018 1172.445 1465.223 1174.652 1476.199 "
                "1181.39 1483.172 1191.826 1599.658 1223.678 1600.644 1239.33 1593.931 1252.575 "
                "1702.41 1317.824 1700.353 1320.683 1696.84 1324.233 1792.963 1402.744 1784.037 "
                "1407.071 1774.453 1408.153 1850.039 1492.3 1844.422 1492.298 1842.468 1492.07 z "
                "m 1185.563 1113.418 c 1269.836 1191.324 1278.847 1187.061 1285.084 1180.534 "
                "1390.581 1241.396 1393.955 1233.927 1394.786 1225.113 1505.12 1262.987 1503.142 "
                "1254.744 1499.423 1248.584 1598.303 1262.519 1588.82 1256.186 1577.763 1254.49 "
                "1653.562 1260.006 1647.705 1260.01 1645.143 1260.402 1698.683 1268.436 1687.328 "
                "1277.944 1682.953 1291.072 1717.956 1330.168 1718.342 1339.009 1721.873 1347.186 "
                "1765.67 1414.597 1775.752 1422.71 1787.77 1425.009 1855.803 1503.24 1859.767 "
                "1503.587 1861.044 1503.477 1934.643 1581.917 1935.798 1581.851 1936.882 1581.798 z";

            expect (Drawable::parseSVGPath (juceLogoSvgPathString).toString() == juceLogoPathString);
        });
    }
};

static SvgParserTests svgParserTests;

#endif

} // namespace juce
