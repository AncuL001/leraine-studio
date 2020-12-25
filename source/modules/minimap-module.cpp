#include "minimap-module.h"

#include <imgui.h>

void MiniMapModule::Generate(Chart* const InChart, Skin& InSkin, const Time InSongLength)
{
    _Width = _BorderPadding * 2 + _NoteWidth + (InChart->KeyAmount * _NoteWidth + (InChart->KeyAmount - 1)) + _NoteWidth;
    _ScaledSongLength = InSongLength / _HeightScale;
    _SongLength = InSongLength;

    _MiniMapRenderTexture.create(_Width, _ScaledSongLength);
    _MiniMapRenderTexture.clear({0, 0, 0, 216});

    InChart->IterateNotesInTimeRange(0, InSongLength, [this, &InSkin, &InSongLength](Note& InOutNote, const Column InColumn)
    {
        if(InOutNote.Type == Note::EType::HoldEnd || InOutNote.Type == Note::EType::HoldIntermediate)
            return;

        if(InOutNote.Type == Note::EType::HoldBegin)
        {
            sf::RectangleShape rectangleHold;

            rectangleHold.setSize(sf::Vector2f(_NoteWidth, (InOutNote.TimePointEnd - InOutNote.TimePointBegin + _NoteHeight) / _HeightScale));
            rectangleHold.setFillColor({32, 255, 32, 255});

            rectangleHold.setPosition(sf::Vector2f(_BorderPadding + _NoteWidth + (InColumn * _NoteWidth + InColumn), (InOutNote.TimePoint - _NoteHeight) / _HeightScale));

            _MiniMapRenderTexture.draw(rectangleHold);
        }

        sf::RectangleShape rectangle;

        rectangle.setSize(sf::Vector2f(_NoteWidth, _NoteHeight));
        rectangle.setFillColor(InSkin.SnapColorTable[InOutNote.BeatSnap]);

        rectangle.setPosition(sf::Vector2f(_BorderPadding + _NoteWidth + (InColumn * _NoteWidth + InColumn), (InOutNote.TimePoint - _NoteHeight) / _HeightScale));

        _MiniMapRenderTexture.draw(rectangle);
    });

    _MiniMapSprite = sf::Sprite();

    _MiniMapSprite.setTexture(_MiniMapRenderTexture.getTexture());
}

TimefieldRenderGraph& MiniMapModule::GetPreviewRenderGraph(Chart* const InChart) 
{
	InChart->IterateNotesInTimeRange(_HoveredTime - _PreviewTimeLength, _HoveredTime + _PreviewTimeLength, [this](Note& InNote, const Column InColumn)
	{
		_PreviewRenderGraph.SubmitNoteRenderCommand(InNote, InColumn);
	});

    return _PreviewRenderGraph;
}

bool MiniMapModule::IsHoveringTimeline(const int InScreenX, const int InScreenY, const int InHeight, const int InDistanceFromBorders, const Time InTime, const Time InTimeScreenBegin, const Time InTimeScreenEnd, const Cursor& InCursor) 
{
    if(_IsDragging)
    {
        _CurrentTime = _HoveredTime;
    }
    else
    {
        _CurrentTime = InTime;
    }

    int resultedHeight = std::min(InHeight - InDistanceFromBorders * 2, (int)_MiniMapRenderTexture.getTexture().getSize().y);

    _TimelinePositionTop = InScreenY + InDistanceFromBorders;
    _TimelinePositionBottom = InScreenY + resultedHeight + InDistanceFromBorders;

    _MiniMapRectangle.setPosition(InScreenX, InScreenY + InDistanceFromBorders);
    _MiniMapRectangle.setSize(sf::Vector2f((float)_MiniMapRenderTexture.getTexture().getSize().x, (float)resultedHeight));

    float percentualDeltaMiniMap = float(resultedHeight) / float(_ScaledSongLength); 
    _SongPositionOnMiniMap = resultedHeight - (_CurrentTime / _HeightScale) * percentualDeltaMiniMap;

    _MiniScreenBottomPosition = resultedHeight - (InTimeScreenBegin / _HeightScale) * percentualDeltaMiniMap;
    _MiniScreenHeight = (float(InTimeScreenBegin) / float(_HeightScale)) - (float(InTimeScreenEnd) / float(_HeightScale));
    

    sf::IntRect subRectangle;

    subRectangle.height = resultedHeight;
    subRectangle.width = _MiniMapRenderTexture.getTexture().getSize().x;

    subRectangle.left = 0;

    float timeOffset = (_CurrentTime / _HeightScale) - (_CurrentTime / _HeightScale) * percentualDeltaMiniMap;
    subRectangle.top = (float(_MiniMapRenderTexture.getTexture().getSize().y) - float(subRectangle.height)) - timeOffset;

    _MiniMapSprite.setTextureRect(subRectangle);

    if(_IsDragging)
        return true;

    if(InCursor.X >= InScreenX && InCursor.X <= InScreenX + _MiniMapRectangle.getSize().x 
    && InCursor.Y >= _TimelinePositionTop && InCursor.Y <= _TimelinePositionBottom)
    {
        int miniMapViewTop = InScreenY + InDistanceFromBorders - subRectangle.top;
        _HoveredTime = (_MiniMapRenderTexture.getTexture().getSize().y - (InCursor.Y - miniMapViewTop)) * _HeightScale;

        if(InCursor.Y >= _MiniMapRectangle.getPosition().y + _MiniScreenBottomPosition + _MiniScreenHeight - _DragButtonBounds && 
           InCursor.Y <= _MiniMapRectangle.getPosition().y + _MiniScreenBottomPosition + _DragButtonBounds / 2)
        {
            _IsPossibleToDrag = true;
            _ShouldPreview = false;
        }
        else
        {
             _ShouldPreview = true;
             _IsPossibleToDrag = false;
             _CursorScreenY = InCursor.Y;
        }

        return true;
    }
    else
    {
        _ShouldPreview = false;
        _IsPossibleToDrag = false;
    }
    

    return false;
}

bool MiniMapModule::IsPossibleToDrag() 
{
    return _IsPossibleToDrag;
}

bool MiniMapModule::IsDragging() 
{
    return _IsDragging;
}

bool MiniMapModule::ShouldPreview() 
{
    return _ShouldPreview;
}

void MiniMapModule::StartDragging() 
{
    if(_IsPossibleToDrag)
        _IsDragging = true;
}

void MiniMapModule::EndDragging() 
{
    _IsDragging = false;
}

Time MiniMapModule::GetHoveredTime() 
{
    return _HoveredTime;
}

void MiniMapModule::Render(sf::RenderTarget* InRenderTarget)
{
    _MiniMapRectangle.setFillColor({0, 0, 0, 0});
    _MiniMapRectangle.setOutlineColor({255, 255, 255, 255});
    _MiniMapRectangle.setOutlineThickness(1.0f);

    _MiniMapSprite.setPosition(_MiniMapRectangle.getPosition().x, _MiniMapRectangle.getPosition().y);

    sf::RectangleShape screenView;

    sf::Vector2f screenViewSize = sf::Vector2f(_MiniMapRectangle.getSize().x, _MiniScreenHeight) * (_IsPossibleToDrag ? 2.0f : 1.0f);

    float xSizeOffset = (screenViewSize.x - _MiniMapRectangle.getSize().x) / 2.f;
    float ySizeOffset = (screenViewSize.y - _MiniScreenHeight) / 2.f;

    screenView.setPosition(_MiniMapRectangle.getPosition().x - xSizeOffset, _MiniMapRectangle.getPosition().y + _MiniScreenBottomPosition - ySizeOffset);
    screenView.setSize(screenViewSize);
    screenView.setFillColor({255, 255, 255, 64});
    screenView.setOutlineColor({255, 255, 255, 255});
    screenView.setOutlineThickness(1.0f);

    InRenderTarget->draw(_MiniMapSprite);
    InRenderTarget->draw(_MiniMapRectangle);
    InRenderTarget->draw(screenView);
}

void MiniMapModule::RenderPreview(sf::RenderTarget* InOutRenderTarget, sf::RenderTexture* InPreviewRenderTexture)
{
     sf::IntRect previewSegment;
    
    previewSegment.width = InPreviewRenderTexture->getSize().x;
    previewSegment.height = 356;

    previewSegment.left = 0;
    previewSegment.top = InOutRenderTarget->getSize().y / 2 - previewSegment.height / 4;
    
    sf::Vector2f position = sf::Vector2f(_MiniMapRectangle.getPosition().x + _MiniMapRectangle.getSize().x + 8, _CursorScreenY - previewSegment.height / 2);

    if(position.y <= _TimelinePositionTop)
        position.y = _TimelinePositionTop;

    if(position.y + previewSegment.height >= _TimelinePositionBottom)
        position.y = _TimelinePositionBottom - previewSegment.height;

    _PreviewSprite.setTexture(InPreviewRenderTexture->getTexture());
    _PreviewSprite.setTextureRect(previewSegment);
    _PreviewSprite.setPosition(position);

    sf::RectangleShape backgroundRectangle;

    backgroundRectangle.setPosition(position);
    backgroundRectangle.setSize(sf::Vector2f(previewSegment.width, previewSegment.height));
    backgroundRectangle.setFillColor({0, 0, 0, 216});
    backgroundRectangle.setOutlineColor({255, 255, 255, 255});
    backgroundRectangle.setOutlineThickness(1.f);

    InOutRenderTarget->draw(backgroundRectangle);
    InOutRenderTarget->draw(_PreviewSprite);

    _PreviewRenderGraph.ClearRenderCommands();
}

bool MiniMapModule::Tick(const float& InDeltaTime) 
{
    if(!_IsDragging)
        return false;

    int relativeMiniMapTime = ImGui::GetMousePos().y - _MiniMapRectangle.getPosition().y;
	_HoveredTime = float(_MiniMapRectangle.getSize().y - relativeMiniMapTime) / _MiniMapRectangle.getSize().y  * float(_SongLength);

    if(_HoveredTime < 0)
        _HoveredTime = 0;

    return true;    
}
