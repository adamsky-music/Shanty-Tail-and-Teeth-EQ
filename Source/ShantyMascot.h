#pragma once

#include <JuceHeader.h>
#include "BinaryData.h"
#include "CustomLookAndFeel.h"
#include <array>
#include <queue>
#include <vector>
#include <cmath>

namespace ShantyEQ
{

class ShantyMascotComponent : public juce::Component
{
public:
    ShantyMascotComponent()
    {
        loadSittingParts();
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void updateAnimation(float teethLevel, float tailLevel, float teethSat, float tailSat, bool sleeping = false)
    {
        isSleeping   = sleeping;
        currentTeeth = juce::jlimit(0.0f, 1.0f, teethLevel);
        currentTail  = juce::jlimit(0.0f, 1.0f, tailLevel);
        currentSat   = juce::jlimit(0.0f, 1.0f, juce::jmax(teethSat, tailSat));
        animTimeSec += 1.0f / 60.0f;

        // Occasional cute ear twitch while sleeping
        if (isSleeping)
        {
            sleepEarTimer -= 1.0f / 60.0f;
            if (sleepEarTimer <= 0.0f)
            {
                sleepEarProgress = 1.0f;
                sleepEarTimer = 2.6f + 2.2f * std::abs(std::sin(animTimeSec * 0.8f));
            }
            if (sleepEarProgress > 0.0f)
                sleepEarProgress = juce::jmax(0.0f, sleepEarProgress - (1.0f / 60.0f) * 2.8f);
        }

        // Snappy ear-twitch envelope driven by Teeth transients
        if (currentTeeth > earEnv)
            earEnv = 0.45f * earEnv + 0.55f * currentTeeth;
        else
            earEnv *= 0.84f;

        // Smooth saturation bite glow envelope
        satEnv = 0.82f * satEnv + 0.18f * currentSat;

        lickTimer -= 1.0f / 60.0f;
        if (lickTimer <= 0.0f)
        {
            lickProgress = 1.0f;
            lickTimer = 2.4f + 1.5f * std::abs(std::cos(animTimeSec * 0.7f));
        }
        if (lickProgress > 0.0f)
            lickProgress = juce::jmax(0.0f, lickProgress - (1.0f / 60.0f) * 1.75f);

        if (isBeingPetted)
            petHappyTimer = 1.6f;
        else if (petHappyTimer > 0.0f)
            petHappyTimer = juce::jmax(0.0f, petHappyTimer - (1.0f / 60.0f));

        for (auto& h : hearts)
        {
            if (h.life > 0.0f)
            {
                h.life -= (1.0f / 60.0f) * 0.9f;
                h.y -= 0.7f;
                h.x += std::sin(animTimeSec * 5.0f + h.phase) * 0.35f;
            }
        }

        if (isBeingPetted)
        {
            heartSpawnTimer -= 1.0f / 60.0f;
            if (heartSpawnTimer <= 0.0f)
            {
                spawnHeart();
                heartSpawnTimer = 0.22f;
            }
        }

        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            isBeingPetted = true;
            petHappyTimer = 1.6f;
            lastPetPos = e.position;
            spawnHeart();
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isBeingPetted)
        {
            if (e.position.getDistanceFrom(lastPetPos) > 18.0f)
            {
                lastPetPos = e.position;
                spawnHeart();
            }
            petHappyTimer = 1.6f;
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isBeingPetted = false;
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        if (!bodyFrame.isValid())
            return;

        if (isSleeping && sleepBodyFrame.isValid())
        {
            drawSleepingShanti(g, bounds);
            return;
        }

        const bool isHappy = (petHappyTimer > 0.01f);
        const float breathY = std::sin(animTimeSec * 2.2f) * 1.0f;

        const auto drawArea = bounds.reduced(4.0f, 4.0f).translated(0.0f, breathY - 2.0f);
        const float imgW = static_cast<float>(bodyFrame.getWidth());
        const float imgH = static_cast<float>(bodyFrame.getHeight());
        const float scale = juce::jmin(1.0f, juce::jmin(drawArea.getWidth() / imgW, drawArea.getHeight() / imgH));
        const float destW = imgW * scale;
        const float destH = imgH * scale;
        const juce::Rectangle<float> destRect(drawArea.getCentreX() - destW * 0.5f,
                                              drawArea.getCentreY() - destH * 0.5f,
                                              destW, destH);

        // Stage glow reacting to Tail (cyan), Teeth (red), and Saturation (amber)
        {
            const float bgCx = destRect.getCentreX();
            const float bgCy = destRect.getCentreY() + destRect.getHeight() * 0.04f;
            const float bgR  = destRect.getWidth() * 0.56f;
            auto glowCol = Palette::tailCyan.interpolatedWith(Palette::teethRed, juce::jlimit(0.0f, 1.0f, earEnv * 1.3f));
            glowCol = glowCol.interpolatedWith(Palette::satAmber, juce::jlimit(0.0f, 1.0f, satEnv * 1.2f));
            glowCol = glowCol.withAlpha(0.14f + 0.28f * juce::jmax(earEnv, juce::jmax(currentTail, satEnv)));
            juce::ColourGradient stageGlow(glowCol, bgCx, bgCy,
                                           juce::Colours::transparentBlack, bgCx + bgR, bgCy, true);
            g.setGradientFill(stageGlow);
            g.fillEllipse(bgCx - bgR, bgCy - bgR, bgR * 2.0f, bgR * 2.0f);
        }

        // Ground shadow
        {
            const float shY = destRect.getBottom() - destRect.getHeight() * 0.14f;
            const auto shadowRect = juce::Rectangle<float>(destRect.getCentreX() - destRect.getWidth() * 0.25f,
                                                           shY,
                                                           destRect.getWidth() * 0.52f,
                                                           destRect.getHeight() * 0.06f);
            g.setColour(juce::Colours::black.withAlpha(0.50f));
            g.fillEllipse(shadowRect);
        }

        g.setOpacity(1.0f);

        // 1. Tail wags in proportion to Tail (Tonal) energy or happy petting
        if (tailFrame.isValid())
        {
            float wagAngle = 0.0f;
            if (isHappy)
                wagAngle = std::sin(animTimeSec * 18.0f) * 0.24f;
            else
                wagAngle = std::sin(animTimeSec * (2.5f + currentTail * 10.0f)) * (0.02f + 0.18f * currentTail);

            const float pivotX = destRect.getX() + 0.305f * destRect.getWidth();
            const float pivotY = destRect.getY() + 0.775f * destRect.getHeight();

            juce::Graphics::ScopedSaveState ss(g);
            g.addTransform(juce::AffineTransform::rotation(wagAngle, pivotX, pivotY));
            g.drawImage(tailFrame, destRect);
        }

        // Subtle alert lean on Teeth (Transient) hits
        const float bounce = earEnv * 0.75f;
        const float groundX = destRect.getCentreX();
        const float groundY = destRect.getBottom() - destRect.getHeight() * 0.13f;
        const auto beatTransform = juce::AffineTransform::scale(1.0f + 0.014f * bounce,
                                                                1.0f - 0.020f * bounce,
                                                                groundX, groundY)
                                       .translated(0.0f, bounce * 1.4f);

        juce::Graphics::ScopedSaveState bodyScope(g);
        g.addTransform(beatTransform);

        // 2. Seated Shanti body
        g.drawImage(bodyFrame, destRect);

        // 2b. Audio-reactive perking/twitching ears on Teeth transients
        if (earsFrame.isValid())
        {
            const float earAngle = -0.095f * earEnv + std::sin(animTimeSec * 22.0f) * (0.025f * earEnv);
            const float earPivotX = destRect.getX() + 0.655f * destRect.getWidth();
            const float earPivotY = destRect.getY() + 0.160f * destRect.getHeight();

            juce::Graphics::ScopedSaveState earScope(g);
            g.addTransform(juce::AffineTransform::rotation(earAngle, earPivotX, earPivotY)
                               .translated(0.0f, -1.6f * earEnv));
            g.drawImage(earsFrame, destRect);
        }

        // 3. Tongue lick
        const float lickEnv = (lickProgress > 0.0f)
            ? std::sin(lickProgress * juce::MathConstants<float>::pi)
            : 0.0f;

        if (lickEnv > 0.02f)
        {
            const float mouthX = destRect.getX() + 0.742f * destRect.getWidth();
            const float mouthY = destRect.getY() + 0.278f * destRect.getHeight();
            const float reach = std::pow(lickEnv, 0.75f);
            const float tipX = mouthX + destRect.getWidth() * (0.008f + 0.036f * reach);
            const float tipY = mouthY + destRect.getHeight() * (0.008f - 0.028f * reach);
            const float baseHalfW = destRect.getHeight() * 0.010f * reach;

            juce::Path tongue;
            tongue.startNewSubPath(mouthX, mouthY - baseHalfW * 0.4f);
            tongue.quadraticTo(mouthX + (tipX - mouthX) * 0.55f, mouthY + destRect.getHeight() * 0.014f * reach,
                               tipX, tipY);
            tongue.quadraticTo(mouthX + (tipX - mouthX) * 0.45f, mouthY + baseHalfW * 1.3f,
                               mouthX - destRect.getWidth() * 0.004f, mouthY + baseHalfW);
            tongue.closeSubPath();

            g.setColour(juce::Colour(0xFFE9798B).withAlpha(juce::jmin(1.0f, lickEnv * 1.4f)));
            g.fillPath(tongue);
            g.setColour(juce::Colour(0xFF421C23).withAlpha(juce::jmin(1.0f, lickEnv * 1.4f)));
            g.strokePath(tongue, juce::PathStrokeType(1.0f));
        }

        // 4. Eyes: Happy "^ ^" when petted, OR sly/fierce focused brow + amber glint when Saturation bites!
        if (isHappy)
        {
            const float happyAlpha = juce::jmin(1.0f, petHappyTimer * 1.5f);
            auto drawHappyEye = [&](float cxNorm, float cyNorm, float rx) {
                const float cx = destRect.getX() + cxNorm * destRect.getWidth();
                const float cy = destRect.getY() + cyNorm * destRect.getHeight();
                g.setColour(juce::Colour(0xFFF6F8FA).withAlpha(happyAlpha));
                g.fillEllipse(cx - rx * 1.35f, cy - rx * 1.15f, rx * 2.7f, rx * 2.3f);

                juce::Path arc;
                arc.startNewSubPath(cx - rx * 1.1f, cy + rx * 0.25f);
                arc.quadraticTo(cx, cy - rx * 0.95f, cx + rx * 1.1f, cy + rx * 0.25f);
                g.setColour(juce::Colour(0xFF22252A).withAlpha(happyAlpha));
                g.strokePath(arc, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };

            const float eyeR = destRect.getWidth() * 0.014f;
            drawHappyEye(0.663f, 0.193f, eyeR);
            drawHappyEye(0.742f, 0.184f, eyeR * 0.85f);
        }
        else if (satEnv > 0.06f)
        {
            const float biteAlpha = juce::jlimit(0.0f, 1.0f, satEnv * 1.4f);
            auto drawBiteBrow = [&](float cxNorm, float cyNorm, float rx, float tilt) {
                const float cx = destRect.getX() + cxNorm * destRect.getWidth();
                const float cy = destRect.getY() + cyNorm * destRect.getHeight();
                // Subtle fierce squint brow
                g.setColour(juce::Colour(0xFF1D2025).withAlpha(0.85f * biteAlpha));
                g.drawLine(cx - rx * 1.1f, cy - rx * (0.8f - tilt), cx + rx * 1.1f, cy - rx * (0.8f + tilt), 1.8f);
                // Warm amber eye glint
                g.setColour(Palette::satAmber.withAlpha(0.75f * biteAlpha));
                g.fillEllipse(cx - rx * 0.35f, cy - rx * 0.35f, rx * 0.7f, rx * 0.7f);
            };
            const float eyeR = destRect.getWidth() * 0.014f;
            drawBiteBrow(0.663f, 0.193f, eyeR, 0.35f);
            drawBiteBrow(0.742f, 0.184f, eyeR * 0.85f, 0.35f);

            // Subtle amber "bite" sparks near snout
            for (int i = 0; i < 3; ++i)
            {
                const float phase = animTimeSec * 9.0f + static_cast<float>(i) * 2.1f;
                const float sx = destRect.getX() + (0.78f + 0.04f * std::sin(phase)) * destRect.getWidth();
                const float sy = destRect.getY() + (0.26f + 0.03f * std::cos(phase * 1.3f)) * destRect.getHeight();
                const float sr = (1.6f + 1.4f * std::abs(std::sin(phase))) * biteAlpha;
                g.setColour(Palette::satAmber.withAlpha(0.8f * biteAlpha));
                g.fillEllipse(sx - sr, sy - sr, sr * 2.0f, sr * 2.0f);
            }
        }

        // 5. Floating hearts
        for (const auto& h : hearts)
        {
            if (h.life > 0.01f)
            {
                const float alpha = juce::jmin(1.0f, h.life * 1.4f);
                const float s = 5.5f;
                juce::Path heart;
                heart.startNewSubPath(h.x, h.y + s * 0.35f);
                heart.cubicTo(h.x - s, h.y - s * 0.4f, h.x - s * 0.5f, h.y - s * 1.1f, h.x, h.y - s * 0.45f);
                heart.cubicTo(h.x + s * 0.5f, h.y - s * 1.1f, h.x + s, h.y - s * 0.4f, h.x, h.y + s * 0.35f);
                heart.closeSubPath();

                g.setColour(Palette::teethRed.withAlpha(alpha));
                g.fillPath(heart);
            }
        }

        // 6. Collar glow in sync with Teeth & Saturation Bite
        const float collarIntensity = juce::jlimit(0.0f, 1.0f, earEnv * 0.85f + satEnv * 0.70f);
        if (collarIntensity > 0.02f && collarGlowImage.isValid())
        {
            const float cx = destRect.getX() + collarNormCenter.x * destRect.getWidth();
            const float cy = destRect.getY() + collarNormCenter.y * destRect.getHeight();
            const float haloRadius = destRect.getWidth() * (0.14f + 0.09f * collarIntensity);

            const auto haloCol = Palette::teethRed.interpolatedWith(Palette::satAmber, satEnv);
            juce::ColourGradient halo(haloCol.withAlpha(0.72f * collarIntensity), cx, cy,
                                      juce::Colours::transparentBlack, cx + haloRadius, cy, true);
            g.setGradientFill(halo);
            g.fillEllipse(cx - haloRadius, cy - haloRadius, haloRadius * 2.0f, haloRadius * 2.0f);

            g.setOpacity(collarIntensity);
            g.drawImage(collarGlowImage, destRect);
            g.setOpacity(1.0f);
        }
    }

private:
    struct FloatingHeart
    {
        float x = 0.0f, y = 0.0f, life = 0.0f, phase = 0.0f;
    };

    juce::Image bodyFrame;
    juce::Image tailFrame;
    juce::Image earsFrame;
    juce::Image collarGlowImage;
    juce::Image sleepBodyFrame;
    juce::Image sleepEarFrame;
    juce::Point<float> collarNormCenter { 0.63f, 0.35f };

    bool isSleeping    = false;
    float sleepEarTimer = 1.8f;
    float sleepEarProgress = 0.0f;
    float currentTeeth = 0.0f;
    float currentTail  = 0.0f;
    float currentSat   = 0.0f;
    float earEnv       = 0.0f;
    float satEnv       = 0.0f;
    float animTimeSec  = 0.0f;

    float lickTimer = 1.2f;
    float lickProgress = 0.0f;
    bool isBeingPetted = false;
    float petHappyTimer = 0.0f;
    float heartSpawnTimer = 0.0f;
    juce::Point<float> lastPetPos;
    std::array<FloatingHeart, 8> hearts {};
    int nextHeartIdx = 0;

    void spawnHeart()
    {
        auto& h = hearts[static_cast<size_t>(nextHeartIdx)];
        nextHeartIdx = (nextHeartIdx + 1) % static_cast<int>(hearts.size());
        h.x = static_cast<float>(getWidth()) * 0.62f + std::sin(animTimeSec * 7.3f + nextHeartIdx) * 14.0f;
        h.y = static_cast<float>(getHeight()) * 0.22f;
        h.life = 1.0f;
        h.phase = animTimeSec;
    }

    static juce::Image stripOuterWhiteBackground(const juce::Image& src)
    {
        const int w = src.getWidth();
        const int h = src.getHeight();
        juce::Image out(juce::Image::ARGB, w, h, true);

        juce::Image::BitmapData srcData(src, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dstData(out, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                dstData.setPixelColour(x, y, srcData.getPixelColour(x, y));

        auto isNearWhite = [&](int x, int y) -> bool {
            const auto c = srcData.getPixelColour(x, y);
            return c.getRed() >= 235 && c.getGreen() >= 235 && c.getBlue() >= 235;
        };

        std::vector<uint8_t> visited(static_cast<size_t>(w * h), 0);
        std::queue<std::pair<int, int>> q;

        for (int x = 0; x < w; ++x)
        {
            if (isNearWhite(x, 0)) { visited[static_cast<size_t>(x)] = 1; q.push({ x, 0 }); }
            if (isNearWhite(x, h - 1)) { visited[static_cast<size_t>((h - 1) * w + x)] = 1; q.push({ x, h - 1 }); }
        }
        for (int y = 1; y < h - 1; ++y)
        {
            if (isNearWhite(0, y)) { visited[static_cast<size_t>(y * w)] = 1; q.push({ 0, y }); }
            if (isNearWhite(w - 1, y)) { visited[static_cast<size_t>(y * w + w - 1)] = 1; q.push({ w - 1, y }); }
        }

        const int dx[4] = { 1, -1, 0, 0 };
        const int dy[4] = { 0, 0, 1, -1 };

        while (!q.empty())
        {
            auto [cx, cy] = q.front();
            q.pop();
            dstData.setPixelColour(cx, cy, juce::Colours::transparentBlack);

            for (int d = 0; d < 4; ++d)
            {
                const int nx = cx + dx[d];
                const int ny = cy + dy[d];
                if (nx >= 0 && nx < w && ny >= 0 && ny < h)
                {
                    const size_t idx = static_cast<size_t>(ny * w + nx);
                    if (!visited[idx] && isNearWhite(nx, ny))
                    {
                        visited[idx] = 1;
                        q.push({ nx, ny });
                    }
                }
            }
        }

        return out;
    }

    static juce::Colour brightenFurPixel(juce::Colour c)
    {
        const int r = c.getRed();
        const int g = c.getGreen();
        const int b = c.getBlue();
        const int maxC = juce::jmax(r, juce::jmax(g, b));
        const int minC = juce::jmin(r, juce::jmin(g, b));
        const int sat = maxC - minC;

        if (sat < 38 && maxC >= 135)
        {
            const float norm = static_cast<float>(maxC - 135) / 120.0f;
            const float boosted = std::pow(juce::jlimit(0.0f, 1.0f, norm), 0.55f);
            const uint8_t nr = static_cast<uint8_t>(juce::jlimit(0, 255, static_cast<int>(185 + boosted * 70.0f)));
            const uint8_t ng = static_cast<uint8_t>(juce::jlimit(0, 255, static_cast<int>(188 + boosted * 67.0f)));
            const uint8_t nb = static_cast<uint8_t>(juce::jlimit(0, 255, static_cast<int>(194 + boosted * 61.0f)));
            return juce::Colour::fromRGBA(nr, ng, nb, c.getAlpha());
        }
        return c;
    }

    void loadSittingParts()
    {
        auto sheet = juce::ImageCache::getFromMemory(BinaryData::shanti_v3_sheet_png,
                                                     BinaryData::shanti_v3_sheet_pngSize);
        if (!sheet.isValid())
            return;

        constexpr int cols = 4;
        constexpr int rows = 3;
        const int cellW = sheet.getWidth() / cols;
        const int cellH = sheet.getHeight() / rows;

        auto cell = sheet.getClippedImage(juce::Rectangle<int>(3 * cellW, 2 * cellH, cellW, cellH));
        auto fullSit = stripOuterWhiteBackground(cell);

        const int w = fullSit.getWidth();
        const int h = fullSit.getHeight();

        bodyFrame = juce::Image(juce::Image::ARGB, w, h, true);
        tailFrame = juce::Image(juce::Image::ARGB, w, h, true);
        earsFrame = juce::Image(juce::Image::ARGB, w, h, true);
        collarGlowImage = juce::Image(juce::Image::ARGB, w, h, true);

        juce::Image::BitmapData srcData(fullSit, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData bodyData(bodyFrame, juce::Image::BitmapData::writeOnly);
        juce::Image::BitmapData tailData(tailFrame, juce::Image::BitmapData::writeOnly);
        juce::Image::BitmapData earsData(earsFrame, juce::Image::BitmapData::writeOnly);
        juce::Image::BitmapData glowData(collarGlowImage, juce::Image::BitmapData::writeOnly);

        const int tailCutX = static_cast<int>(w * 0.305f);
        const int tailCutY = static_cast<int>(h * 0.500f);
        const int earMinX  = static_cast<int>(w * 0.560f);
        const int earMaxX  = static_cast<int>(w * 0.780f);
        const int earMaxY  = static_cast<int>(h * 0.160f);

        double sumX = 0.0, sumY = 0.0;
        int count = 0;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const auto raw = srcData.getPixelColour(x, y);
                if (raw.getAlpha() == 0)
                    continue;

                const auto c = brightenFurPixel(raw);

                if (x < tailCutX && y > tailCutY)
                    tailData.setPixelColour(x, y, c);
                else if (x >= earMinX && x <= earMaxX && y < earMaxY)
                    earsData.setPixelColour(x, y, c);
                else
                    bodyData.setPixelColour(x, y, c);

                if (raw.getAlpha() > 128)
                {
                    const int r = raw.getRed();
                    const int g = raw.getGreen();
                    const int b = raw.getBlue();
                    if (r >= 120 && r > static_cast<int>(g * 1.55f) && r > static_cast<int>(b * 1.55f))
                    {
                        glowData.setPixelColour(x, y, juce::Colour(0xFFFF424E));
                        sumX += x;
                        sumY += y;
                        ++count;
                    }
                }
            }
        }

        if (count > 0)
            collarNormCenter = { static_cast<float>(sumX / (count * w)),
                                 static_cast<float>(sumY / (count * h)) };

        // Load curled-up sleeping Shanti
        auto sleepSrc = juce::ImageCache::getFromMemory(BinaryData::shanti_sleep_png,
                                                        BinaryData::shanti_sleep_pngSize);
        if (sleepSrc.isValid())
        {
            auto cleanSleep = stripOuterWhiteBackground(sleepSrc);
            const int sw = cleanSleep.getWidth();
            const int sh = cleanSleep.getHeight();
            sleepBodyFrame = juce::Image(juce::Image::ARGB, sw, sh, true);
            sleepEarFrame  = juce::Image(juce::Image::ARGB, sw, sh, true);

            juce::Image::BitmapData sSrc(cleanSleep, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData sBody(sleepBodyFrame, juce::Image::BitmapData::writeOnly);
            juce::Image::BitmapData sEar(sleepEarFrame, juce::Image::BitmapData::writeOnly);

            const int sEarMinX = static_cast<int>(sw * 0.62f);
            const int sEarMaxX = static_cast<int>(sw * 0.94f);
            const int sEarMaxY = static_cast<int>(sh * 0.47f);

            for (int y = 0; y < sh; ++y)
            {
                for (int x = 0; x < sw; ++x)
                {
                    const auto raw = sSrc.getPixelColour(x, y);
                    if (raw.getAlpha() == 0)
                        continue;
                    const auto c = brightenFurPixel(raw);
                    if (x >= sEarMinX && x <= sEarMaxX && y < sEarMaxY)
                        sEar.setPixelColour(x, y, c);
                    else
                        sBody.setPixelColour(x, y, c);
                }
            }
        }
    }

    void drawSleepingShanti(juce::Graphics& g, const juce::Rectangle<float>& bounds) const
    {
        const float breath = 0.5f + 0.5f * std::sin(animTimeSec * 1.6f);
        const auto drawArea = bounds.reduced(6.0f, 6.0f).translated(0.0f, 10.0f);
        const float imgW = static_cast<float>(sleepBodyFrame.getWidth());
        const float imgH = static_cast<float>(sleepBodyFrame.getHeight());
        const float scale = juce::jmin(1.0f, juce::jmin(drawArea.getWidth() / imgW, drawArea.getHeight() / imgH));
        const float destW = imgW * scale;
        const float destH = imgH * scale;
        const juce::Rectangle<float> destRect(drawArea.getCentreX() - destW * 0.5f,
                                              drawArea.getCentreY() - destH * 0.5f,
                                              destW, destH);

        // Soft ambient stage glow
        {
            const float bgCx = destRect.getCentreX();
            const float bgCy = destRect.getCentreY() + destRect.getHeight() * 0.08f;
            const float bgR  = destRect.getWidth() * 0.52f;
            juce::ColourGradient stageGlow(Palette::tailCyan.withAlpha(0.10f), bgCx, bgCy,
                                           juce::Colours::transparentBlack, bgCx + bgR, bgCy, true);
            g.setGradientFill(stageGlow);
            g.fillEllipse(bgCx - bgR, bgCy - bgR, bgR * 2.0f, bgR * 2.0f);
        }

        // Ground shadow
        {
            const float shY = destRect.getY() + destRect.getHeight() * 0.72f;
            const auto shadowRect = juce::Rectangle<float>(destRect.getX() + destRect.getWidth() * 0.08f,
                                                           shY,
                                                           destRect.getWidth() * 0.84f,
                                                           destRect.getHeight() * 0.12f);
            g.setColour(juce::Colours::black.withAlpha(0.48f));
            g.fillEllipse(shadowRect);
        }

        // Gentle sleeping chest/body breathing
        const float groundX = destRect.getCentreX();
        const float groundY = destRect.getY() + destRect.getHeight() * 0.76f;
        const auto breathTx = juce::AffineTransform::scale(1.0f + 0.012f * breath,
                                                           1.0f - 0.016f * breath,
                                                           groundX, groundY);
        {
            juce::Graphics::ScopedSaveState ss(g);
            g.addTransform(breathTx);
            g.setOpacity(1.0f);
            g.drawImage(sleepBodyFrame, destRect);

            if (sleepEarFrame.isValid())
            {
                const float twitch = (sleepEarProgress > 0.0f)
                    ? std::sin(sleepEarProgress * juce::MathConstants<float>::twoPi * 2.0f) * 0.075f * sleepEarProgress
                    : 0.0f;
                const float earPx = destRect.getX() + 0.74f * destRect.getWidth();
                const float earPy = destRect.getY() + 0.44f * destRect.getHeight();
                juce::Graphics::ScopedSaveState earSs(g);
                g.addTransform(juce::AffineTransform::rotation(twitch, earPx, earPy));
                g.drawImage(sleepEarFrame, destRect);
            }
        }

        // Soft floating "z z Z"
        const char* zs[3] = { "z", "z", "Z" };
        for (int i = 0; i < 3; ++i)
        {
            const float cycle = std::fmod(animTimeSec * 0.42f + static_cast<float>(i) * 0.33f, 1.0f);
            const float alpha = std::sin(cycle * juce::MathConstants<float>::pi) * 0.65f;
            const float zx = destRect.getX() + (0.72f + 0.08f * cycle + static_cast<float>(i) * 0.03f) * destRect.getWidth();
            const float zy = destRect.getY() + (0.34f - 0.22f * cycle) * destRect.getHeight();
            g.setColour(Palette::tailCyan.withAlpha(alpha));
            g.setFont(juce::FontOptions(11.0f + static_cast<float>(i) * 2.0f, juce::Font::bold));
            g.drawText(zs[i], static_cast<int>(zx - 8), static_cast<int>(zy - 8), 16, 16, juce::Justification::centred);
        }
    }
};

} // namespace ShantyEQ
