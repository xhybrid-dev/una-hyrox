/**
 ******************************************************************************
 * @file    GpxReader.cpp
 * @brief   The streaming GPX point reader (see the header).
 ******************************************************************************
 */

#include "GpxReader.hpp"

#include <cstring>

namespace Trail
{

namespace
{
bool isBlank(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

/// Element or attribute name without a namespace prefix: "gpx:trkpt" -> "trkpt".
void localName(const char*& s, size_t& n)
{
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == ':') {
            s += i + 1;
            n -= i + 1;
            return;
        }
    }
}

bool nameIs(const char* s, size_t n, const char* want)
{
    const size_t w = std::strlen(want);
    if (n != w) {
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');   // GPX is lower case; tolerate hand-made files
        }
        if (c != want[i]) {
            return false;
        }
    }
    return true;
}

constexpr int64_t kMaxEleCm = 2000000;   // 20 km: anything beyond is not an elevation
} // namespace

GpxReader::GpxReader(GpxSink& sink)
    : mSink(sink)
{
}

void GpxReader::reset()
{
    mStats    = Stats {};
    mState    = State::Text;
    mQuote    = 0;
    mTagLen   = 0;
    mTagSeen  = 0;
    mCapture  = Capture::None;
    mTextLen  = 0;
    mInPoint  = false;
    mPointOk  = false;
    mHasEle   = false;
    mWptDepth = 0;
    mHaveName = false;
}

bool GpxReader::parseFixed(const char* s, size_t len, unsigned digits, int64_t& out)
{
    size_t i = 0;
    while (i < len && isBlank(s[i])) {
        ++i;
    }
    while (len > i && isBlank(s[len - 1])) {
        --len;
    }
    bool neg = false;
    if (i < len && (s[i] == '-' || s[i] == '+')) {
        neg = s[i] == '-';
        ++i;
    }

    int64_t  value   = 0;
    unsigned frac    = 0;       // fractional digits taken
    bool     any     = false;
    bool     point   = false;
    bool     roundUp = false;
    for (; i < len; ++i) {
        const char c = s[i];
        if (c == '.' && !point) {
            point = true;
            continue;
        }
        if (!isDigit(c)) {
            return false;
        }
        any = true;
        if (!point) {
            if (value > 100000000) {   // far past any coordinate or elevation: refuse, don't overflow
                return false;
            }
            value = value * 10 + (c - '0');
        } else if (frac < digits) {
            value = value * 10 + (c - '0');
            ++frac;
        } else if (frac == digits) {
            roundUp = c >= '5';        // the first digit past the precision rounds; the rest are ignored
            ++frac;
        }
    }
    if (!any) {
        return false;
    }
    for (; frac < digits; ++frac) {
        value *= 10;
    }
    if (roundUp) {
        ++value;
    }
    out = neg ? -value : value;
    return true;
}

void GpxReader::feed(const char* data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        const char c = data[i];
        if (mState == State::Text) {
            if (c == '<') {
                mState   = State::Tag;
                mQuote   = 0;
                mTagLen  = 0;
                mTagSeen = 0;
                mTail[0] = mTail[1] = 0;
            } else if (mCapture != Capture::None) {
                appendText(&c, 1);
            }
            continue;
        }

        // In a tag. Comments, CDATA and <!DOCTYPE ..> start with '!': quotes
        // mean nothing there (a comment's "don't" must not open a string).
        const bool special = mTagSeen > 0 && mTag[0] == '!';
        if (mQuote != 0) {
            if (c == mQuote) {
                mQuote = 0;
            }
        } else if (c == '>') {
            const bool comment = mTagSeen >= 3 && std::strncmp(mTag, "!--", 3) == 0;
            const bool cdata   = mTagSeen >= 8 && std::strncmp(mTag, "![CDATA[", 8) == 0;
            if ((comment && !(mTagSeen >= 5 && mTail[0] == '-' && mTail[1] == '-'))
                || (cdata && !(mTail[0] == ']' && mTail[1] == ']'))) {
                appendTag(c);   // a '>' inside a comment or CDATA: keep going
                continue;
            }
            endTag();
            mState = State::Text;
            continue;
        } else if ((c == '"' || c == '\'') && !special && mTagSeen > 0) {
            mQuote = c;
        }
        appendTag(c);
    }
}

void GpxReader::appendTag(char c)
{
    if (mTagLen < kTagMax - 1) {
        mTag[mTagLen++] = c;
    }
    ++mTagSeen;
    mTail[0] = mTail[1];
    mTail[1] = c;
}

void GpxReader::appendText(const char* s, size_t n)
{
    for (size_t i = 0; i < n && mTextLen < kTextMax - 1; ++i) {
        mText[mTextLen++] = s[i];
    }
}

void GpxReader::endTag()
{
    if (mTagSeen > mTagLen) {
        ++mStats.longTags;
    }
    mTag[mTagLen] = '\0';
    if (mTagLen == 0) {
        return;
    }

    if (mTag[0] == '!') {
        // CDATA's text counts as text (a name can be wrapped in it); the rest is skipped.
        if (mCapture != Capture::None && mTagLen >= 10 && std::strncmp(mTag, "![CDATA[", 8) == 0) {
            const bool cut = mTagSeen > mTagLen;   // no closing "]]" stored to drop
            appendText(mTag + 8, mTagLen - (cut ? 8 : 10));
        }
        return;
    }
    if (mTag[0] == '?') {
        return;
    }

    const bool closing = mTag[0] == '/';
    size_t     end     = mTagLen;
    while (end > 0 && isBlank(mTag[end - 1])) {
        --end;
    }
    const bool selfClosing = !closing && end > 0 && mTag[end - 1] == '/';

    const char* name = mTag + (closing ? 1 : 0);
    size_t      n    = 0;
    while (name + n < mTag + mTagLen && !isBlank(name[n]) && name[n] != '/' && name[n] != '>') {
        ++n;
    }
    localName(name, n);

    if (nameIs(name, n, "trkpt") || nameIs(name, n, "rtept")) {
        const PointKind kind = nameIs(name, n, "trkpt") ? PointKind::Track : PointKind::Route;
        if (closing) {
            if (mInPoint) {
                closePoint();
            }
        } else {
            openPoint(kind, selfClosing);
        }
    } else if (nameIs(name, n, "ele")) {
        if (!closing && !selfClosing && mInPoint) {
            mCapture = Capture::Ele;
            mTextLen = 0;
        } else if (closing && mCapture == Capture::Ele) {
            int64_t cm = 0;
            if (parseFixed(mText, mTextLen, 2, cm) && cm > -kMaxEleCm && cm < kMaxEleCm) {
                mHasEle = true;
                mEleCm  = static_cast<int32_t>(cm);
            }
            mCapture = Capture::None;
        }
    } else if (nameIs(name, n, "name")) {
        if (!closing && !selfClosing && !mHaveName && !mInPoint && mWptDepth == 0) {
            mCapture = Capture::Name;
            mTextLen = 0;
        } else if (closing && mCapture == Capture::Name) {
            finishName();
            mCapture = Capture::None;
        }
    } else if (nameIs(name, n, "wpt")) {
        if (!closing) {
            ++mStats.waypoints;
            if (!selfClosing) {
                ++mWptDepth;
            }
        } else if (mWptDepth > 0) {
            --mWptDepth;
        }
    } else if (nameIs(name, n, "gpx")) {
        if (!closing) {
            mStats.sawGpx = true;
        }
    }
}

bool GpxReader::attribute(const char* key, const char*& value, size_t& len) const
{
    // Skip the element name, then walk name="value" pairs.
    size_t i = 0;
    while (i < mTagLen && !isBlank(mTag[i]) && mTag[i] != '/') {
        ++i;
    }
    while (i < mTagLen) {
        while (i < mTagLen && (isBlank(mTag[i]) || mTag[i] == '/')) {
            ++i;
        }
        const size_t nameStart = i;
        while (i < mTagLen && !isBlank(mTag[i]) && mTag[i] != '=') {
            ++i;
        }
        const char* an = mTag + nameStart;
        size_t      al = i - nameStart;
        while (i < mTagLen && isBlank(mTag[i])) {
            ++i;
        }
        if (i >= mTagLen || mTag[i] != '=') {
            continue;   // a bare word: not an attribute we can use
        }
        ++i;
        while (i < mTagLen && isBlank(mTag[i])) {
            ++i;
        }
        if (i >= mTagLen || (mTag[i] != '"' && mTag[i] != '\'')) {
            return false;   // unquoted value: not XML
        }
        const char   q     = mTag[i++];
        const size_t start = i;
        while (i < mTagLen && mTag[i] != q) {
            ++i;
        }
        if (i >= mTagLen) {
            return false;   // cut off by a long tag
        }
        localName(an, al);
        if (nameIs(an, al, key)) {
            value = mTag + start;
            len   = i - start;
            return true;
        }
        ++i;
    }
    return false;
}

void GpxReader::openPoint(PointKind kind, bool selfClosing)
{
    mInPoint = true;
    mKind    = kind;
    mHasEle  = false;
    mEleCm   = 0;
    mCapture = Capture::None;

    const char* v   = nullptr;
    size_t      len = 0;
    int64_t     lat = 0;
    int64_t     lon = 0;
    mPointOk = attribute("lat", v, len) && parseFixed(v, len, 7, lat) && attribute("lon", v, len)
               && parseFixed(v, len, 7, lon);
    if (mPointOk) {
        mPoint.latE7 = lat >= -Geo::kMaxLatE7 && lat <= Geo::kMaxLatE7 ? static_cast<int32_t>(lat) : 0;
        mPoint.lonE7 = lon >= -Geo::kMaxLonE7 && lon <= Geo::kMaxLonE7 ? static_cast<int32_t>(lon) : 0;
        mPointOk = lat >= -Geo::kMaxLatE7 && lat <= Geo::kMaxLatE7 && lon >= -Geo::kMaxLonE7
                   && lon <= Geo::kMaxLonE7;
    }
    if (selfClosing) {
        closePoint();
    }
}

void GpxReader::closePoint()
{
    if (mPointOk) {
        if (mKind == PointKind::Track) {
            ++mStats.trackPoints;
        } else {
            ++mStats.routePoints;
        }
        mSink.point(mPoint, mHasEle, mEleCm, mKind);
    } else {
        ++mStats.badPoints;
    }
    mInPoint = false;
    mCapture = Capture::None;
}

void GpxReader::finishName()
{
    // Decode the five XML entities and numeric references; trim blanks.
    size_t out = 0;
    for (size_t i = 0; i < mTextLen && out < kNameMax - 1;) {
        char c = mText[i];
        if (c == '&') {
            size_t semi = i + 1;
            while (semi < mTextLen && semi - i < 10 && mText[semi] != ';') {
                ++semi;
            }
            if (semi < mTextLen && mText[semi] == ';') {
                const char*  e  = mText + i + 1;
                const size_t el = semi - i - 1;
                char         d  = 0;
                if (el == 3 && std::strncmp(e, "amp", 3) == 0)       { d = '&'; }
                else if (el == 2 && std::strncmp(e, "lt", 2) == 0)   { d = '<'; }
                else if (el == 2 && std::strncmp(e, "gt", 2) == 0)   { d = '>'; }
                else if (el == 4 && std::strncmp(e, "quot", 4) == 0) { d = '"'; }
                else if (el == 4 && std::strncmp(e, "apos", 4) == 0) { d = '\''; }
                else if (el >= 2 && e[0] == '#')                     { d = '?'; }   // non-ASCII in practice
                if (d != 0) {
                    mStats.name[out++] = d;
                    i                  = semi + 1;
                    continue;
                }
            }
        }
        if (c == '\n' || c == '\r' || c == '\t') {
            c = ' ';
        }
        if (!(c == ' ' && (out == 0 || mStats.name[out - 1] == ' '))) {
            mStats.name[out++] = c;
        }
        ++i;
    }
    while (out > 0 && mStats.name[out - 1] == ' ') {
        --out;
    }
    mStats.name[out] = '\0';
    mHaveName        = out > 0;
}

} // namespace Trail
