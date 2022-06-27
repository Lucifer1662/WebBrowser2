#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <optional>
#include <functional>
#include <map>

#include "poly_iterator.h"
using BoundingBox = glm::ivec2;
using Vec2 = glm::ivec2;
using Colour = glm::vec3;


struct Font {
	virtual BoundingBox CharaterBoundingBox(char c, size_t fontSize, bool bold, bool italics) = 0;
};


struct Style {
	size_t fontSize;
	Colour colour;
	bool bold;
	bool italics;
	std::shared_ptr<Font> font;
};


struct FontDrawingConext;

struct DrawingContext {
	virtual void DrawCharacter(char c) = 0;
	virtual void DrawCharacter(char c, BoundingBox& boundingBox) = 0;
	virtual std::unique_ptr<FontDrawingConext> CreateFontContext() = 0;
};

struct DrawingConextDecorator : public DrawingContext {
	DrawingContext& child;
	DrawingConextDecorator(DrawingContext& child) :child(child) {}
	virtual void DrawCharacter(char c) {
		child.DrawCharacter(c);
	}
	virtual void DrawCharacter(char c, BoundingBox& boundingBox) {
		child.DrawCharacter(c, boundingBox);
	}
};

struct FontDrawingConext : public DrawingConextDecorator {
	FontDrawingConext(DrawingContext& context) : DrawingConextDecorator(context) {};

	virtual void SetFontSize(size_t) = 0;
	virtual void SetColour(Colour) = 0;
	virtual void SetBold(bool) = 0;
	virtual void SetItalics(bool) = 0;
	virtual void SetFont(std::shared_ptr<Font> font) = 0;
};


 
struct Glyph {
	virtual void Draw(DrawingContext&) {}
	virtual BoundingBox Bounds() { return { 0,0 }; }
	virtual Vec2 Position() { return { 0,0 }; }
	virtual Vec2 RelativePosition() { return { 0,0 }; }
	virtual void SetPosition(const Vec2& position) = 0;
	virtual void SetWidth(std::optional<size_t> width) = 0;
	virtual void SetHeight(std::optional<size_t> height) = 0;
	virtual void OnBoundsChange(std::function<void(BoundingBox, BoundingBox)>) {};
	
	virtual poly_const_iterator begin() = 0;
	virtual poly_const_iterator end() = 0;
	virtual size_t children() { return 0; }
	virtual Glyph& Add(std::unique_ptr<Glyph>&& glyph) { return *glyph; }
	virtual Glyph& Add(std::unique_ptr<Glyph>&& glyph, int index) { return *glyph; }
	virtual void Remove(poly_const_iterator index) { };
	virtual void Remove(poly_const_iterator begin, poly_const_iterator end) { };


	virtual ~Glyph() = default;
};

struct GlyphDecorator : public Glyph {
	std::unique_ptr<Glyph> glyph;
	GlyphDecorator(std::unique_ptr<Glyph>&& glyph) : glyph(std::move(glyph)) {}
	virtual void Draw(DrawingContext& c) { glyph->Draw(c); }
	virtual BoundingBox Bounds() { return glyph->Bounds();  }
	virtual Vec2 Position() { return  glyph->Position(); }
	virtual Vec2 RelativePosition() { return  glyph->RelativePosition(); }
	virtual void SetPosition(const Vec2& position) { glyph->SetPosition(position); };
	virtual void SetWidth(std::optional<size_t> width) { glyph->SetWidth(width); };
	virtual void SetHeight(std::optional<size_t> height) { glyph->SetHeight(height); };
	virtual void OnBoundsChange(std::function<void(BoundingBox, BoundingBox)> f) { glyph->OnBoundsChange(f); };

	virtual poly_const_iterator begin() { return glyph->begin(); };
	virtual poly_const_iterator end() { return glyph->end(); };
	virtual size_t children() { return glyph->children(); };
	virtual Glyph& Add(std::unique_ptr<Glyph>&& glyph) { glyph->Add(std::move(glyph)); };
	virtual void Remove(poly_const_iterator index) { glyph->Remove(index);  };
	virtual void Remove(poly_const_iterator begin, poly_const_iterator end) { glyph->Remove(begin, end); };

};

struct GlyphDecoratorNonOwner : public Glyph {
	Glyph* glyph;
	GlyphDecoratorNonOwner(Glyph* glyph) : glyph(glyph) {}
	virtual void Draw(DrawingContext& c) { glyph->Draw(c); }
	virtual BoundingBox Bounds() { return glyph->Bounds(); }
	virtual Vec2 Position() { return  glyph->Position(); }
	virtual Vec2 RelativePosition() { return  glyph->RelativePosition(); }
	virtual void SetPosition(const Vec2& position) { glyph->SetPosition(position); };
	virtual void SetWidth(std::optional<size_t> width) { glyph->SetWidth(width); };
	virtual void SetHeight(std::optional<size_t> height) { glyph->SetHeight(height); };
	virtual void OnBoundsChange(std::function<void(BoundingBox, BoundingBox)> f) { glyph->OnBoundsChange(f); };

	virtual poly_const_iterator begin() { return glyph->begin(); };
	virtual poly_const_iterator end() { return glyph->end(); };
	virtual size_t children() { return glyph->children(); };
	virtual Glyph& Add(std::unique_ptr<Glyph>&& glyph) { glyph->Add(std::move(glyph)); };
	virtual void Remove(poly_const_iterator index) { glyph->Remove(index); };
	virtual void Remove(poly_const_iterator begin, poly_const_iterator end) { glyph->Remove(begin, end); };

};

template<typename Container = std::vector<std::unique_ptr<Glyph>>>
class GlyphComposite : public Glyph {
private:
	std::vector<std::unique_ptr<Glyph>> mGlyphs;

public:
	Glyph& Add(std::unique_ptr<Glyph>&& glyph) {
		mGlyphs.push_back(std::move(glyph));
	}

	Glyph& Add(std::unique_ptr<Glyph>&& glyph, int index) {
		mGlyphs.insert(mGlyphs.begin()+index, std::move(glyph));
	}


	void Draw(DrawingContext& context) override {
		for(auto& glyph : mGlyphs)
			glyph->Draw(context);
	};

	virtual void SetWidth(std::optional<size_t> width) {
		for (auto& glyph : mGlyphs) {
			glyph->SetWidth(width);
		}
	}

	virtual void SetHeight(std::optional<size_t> height) {
		for (auto& glyph : mGlyphs) {
			glyph->SetHeight(height);
		}
	}

	poly_const_iterator begin() {
		return mGlyphs.begin();
	}

	poly_const_iterator end() {
		return mGlyphs.end();
	}

	void Remove(poly_const_iterator it) override { 
		using Impl = poly_const_iterator::model<std::vector<std::unique_ptr<Glyph>>::iterator>;
		mGlyphs.erase(((Impl*)it._impl.get())->_iter);
	}

	void Remove(poly_const_iterator begin, poly_const_iterator end) override {
		using Impl = poly_const_iterator::model<std::vector<std::unique_ptr<Glyph>>::iterator>;
		mGlyphs.erase(((Impl*)begin._impl.get())->_iter, ((Impl*)end._impl.get())->_iter);
	};


	size_t children() { return mGlyphs.size(); }
};

struct RowGlyph : public GlyphComposite<> {
	BoundingBox mBounds;
	Vec2 mPosition;
	bool useBoundingWidth;
	size_t mGap;
	BoundingBox Bounds() override { return mBounds; }
	Vec2 Position() override { return mPosition; }
	void SetPosition(const Vec2& position) override {
		mPosition = position;
		auto pos = position;
		for (auto& glyph : *this) {
			glyph->SetPosition(pos);
			pos.x += glyph->Bounds().x;
		}
	}
	void SetWidth(std::optional<size_t> width) override {
		if (width) {
			if (!useBoundingWidth || mBounds.x != *width) {
				mBounds.x = *width;
				useBoundingWidth = true;
				auto pos = mPosition;
				size_t maxHeight = 0;
				size_t lastLineMaxHeight = 0;
				for (auto& glyph : *this) {
					auto gBounds = glyph->Bounds();
					if (gBounds.x || pos.x > mBounds.x) {
						pos.x = mPosition.x;
						pos.y += mGap + lastLineMaxHeight;
						lastLineMaxHeight = maxHeight;
						maxHeight = 0;
					}
					if (gBounds.y > maxHeight) {
						maxHeight = gBounds.y;
					}
					glyph->SetPosition(pos);
					pos.x += gBounds.x;
				}
			}
		}
		else {
			if (useBoundingWidth) {
				useBoundingWidth = false;
				auto pos = mPosition;
				for (auto& glyph : *this) {
					glyph->SetPosition(pos);
					pos.x += glyph->Bounds().x;
				}
			}
		}
	}
	void SetHeight(std::optional<size_t> height) override {
		GlyphComposite::SetHeight(height);
	}
};

struct ColumnGlyph : public GlyphComposite<> {
	BoundingBox mBounds;
	Vec2 mPosition;
	virtual BoundingBox Bounds() { return mBounds; }
	virtual Vec2 Position() { return mPosition; }
	void SetPosition(const Vec2& position) override {
		mPosition = position;
		auto pos = position;
		for (auto& glyph : *this) {
			glyph->SetPosition(pos);
			pos.y += glyph->Bounds().y;
		}
	}
	virtual void SetHeight(std::optional<size_t> height) {
		if (height) {
			mBounds.y = *height;
			GlyphComposite::SetHeight(mBounds.y / children());
		}
		else {
			GlyphComposite::SetHeight(height);
		}
	}
	virtual void SetWidth(std::optional<size_t> width) {
		GlyphComposite::SetWidth(width);
	}
};


struct CharacterGlyph : public Glyph {
protected:
	char c;
	Vec2 mPosition;
public:
	CharacterGlyph(char c) :c(c) {}

	void Draw(DrawingContext& context) override {
		context.DrawCharacter(c);
	};
	
	virtual Vec2 Position() { return mPosition; }
	virtual void SetPosition(Vec2 position) {
		mPosition = position;
	}
};



struct TextCharacterGlyph : public CharacterGlyph {
	Style& style;
public:
	void Draw(DrawingContext& context) override {
		context.DrawCharacter(c);
	};

	virtual BoundingBox Bounds() { return style.font->CharaterBoundingBox(c, style.fontSize, style.bold, style.italics); }
};





class TextGlyph : public Glyph {
	std::string mText;
	std::map<size_t, std::unique_ptr<Glyph>> mGlyphs;
	std::shared_ptr<Style> mStyle;
	BoundingBox mBounds;
	bool useBoundingWidth = false;
	Vec2 mPosition;
	size_t lineSpacing = 1;
public:	
	void Draw(DrawingContext& context) override {
		auto fontContext = context.CreateFontContext();
		for (auto& [i,glyph] : mGlyphs) {
			glyph->Draw(*fontContext);
		}
	};

	BoundingBox Bounds() override { return mBounds; }
	Vec2 Position() override { return mPosition; }
	void SetPosition(const Vec2& position) override {
		mPosition = position;
		auto pos = position;
		for (auto& [i,glyph] : mGlyphs) {
			glyph->SetPosition(pos);
			pos.x += glyph->Bounds().x;
		}
	}
	void SetWidth(std::optional<size_t> width) override {
		if (width) {
			if (!useBoundingWidth || mBounds.x != *width) {
				mBounds.x = *width;
				useBoundingWidth = true;
				auto pos = mPosition;
				size_t maxHeight = 0;
				size_t lastLineMaxHeight = 0;
				for (auto& [i, glyph] : mGlyphs) {
					auto gBounds = glyph->Bounds();
					if (gBounds.x || pos.x > mBounds.x) {
						pos.x = mPosition.x;
						pos.y += lineSpacing + lastLineMaxHeight;
						lastLineMaxHeight = maxHeight;
						maxHeight = 0;
					}
					if (gBounds.y > maxHeight) {
						maxHeight = gBounds.y;
					}
					glyph->SetPosition(pos);
					pos.x += gBounds.x;
				}
			}
		
		}
		else {
			if (useBoundingWidth) {
				useBoundingWidth = false;
				auto pos = mPosition;
				for (auto& [i,glyph] : mGlyphs) {
					glyph->SetPosition(pos);
					pos.x += glyph->Bounds().x;
				}
			}
		}
	}


	void SetText(const std::string& text) {}
	void InsertText(size_t position, const std::string& text) {}
	void RemoveText(size_t start, size_t end) {}
};
