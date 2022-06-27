#pragma once
#include "glyph.h"
#include <memory>
#include <string>
#include <unordered_map>

struct Composition;

using Iterator = std::vector<std::unique_ptr<Glyph>>::iterator;

struct Compositor {
protected:
	Composition* mComposition;
public:
	virtual void ElementsAdded(Iterator begin, Iterator end) {}
	virtual void ElementRemoved(Iterator begin, Iterator end) {}
	virtual void Compose() {}
	void SetComposition(Composition* composition) { mComposition = composition; }
};

struct GlyphParent : public GlyphDecoratorNonOwner {
	Glyph& mParent;
	GlyphParent(Glyph& parent, Glyph* child): mParent(parent), GlyphDecoratorNonOwner(child) {}
};


class Composition : public Glyph {
private:
	std::vector<std::unique_ptr<Glyph>> mGlyphs;
	std::unique_ptr<Glyph> mSelfGlyph;

public:
	std::unique_ptr<Glyph>& SelfGlyph() { return mSelfGlyph; };
	std::vector<std::unique_ptr<Glyph>>& RawGlyphs() { return mGlyphs; };


	Glyph& Add(std::unique_ptr<Glyph>&& glyph) override {
		mGlyphs.push_back(std::move(glyph));
	}

	Glyph& Add(std::unique_ptr<Glyph>&& glyph, int index) override {
		mGlyphs.insert(mGlyphs.begin() + index, std::move(glyph));
	}


	void Draw(DrawingContext& context) override {
		if (mSelfGlyph) {
			mSelfGlyph->Draw(context);
		}
	};

	poly_const_iterator begin() {
		return mSelfGlyph->begin();
	}

	poly_const_iterator end() {
		return mSelfGlyph->end();
	}

	size_t children() { return mSelfGlyph->children(); }
};


struct WrapCompositor : public Compositor {
	size_t mGap = 0;
	std::unordered_map<Glyph*, std::tuple<RowGlyph*, size_t, size_t, size_t>> mIndex;

	void Compose() override {
		mComposition->SelfGlyph() = std::make_unique<ColumnGlyph>();
		auto& column = *mComposition->SelfGlyph();
		auto& row = column.Add(std::make_unique<RowGlyph>());

		auto width = mComposition->Bounds().x;
		size_t i = 0;
		for (auto& glyph : mComposition->RawGlyphs()) {
			if (row.children() == 0) {
				row.Add(std::make_unique<GlyphDecoratorNonOwner>(glyph.get()));
			}else {
				auto gBounds = glyph->Bounds();
				if (row.Bounds().x + gBounds.x < width) {
					row.Add(std::make_unique<GlyphDecoratorNonOwner>(glyph.get()));
				}
				else {
					row = column.Add(std::make_unique<RowGlyph>());
					row.Add(std::make_unique<GlyphDecoratorNonOwner>(glyph.get()));
				}
			}
			mIndex[glyph.get()] = { (RowGlyph*)&row, row.children() - 1, i, column.children()-1 };
			i++;
		}
	}

	virtual void ElementAdded(Iterator begin, Iterator end) {
		RowGlyph* row;
		{
			int index = begin - mComposition->RawGlyphs().begin();
			size_t rowsIndex;
			size_t rawIndex;
			if (index = 0) {
				row = (RowGlyph*)mComposition->SelfGlyph()->begin().operator->().get();
				rawIndex = 0;
				rowsIndex = 0;
			}
			else {
				auto [nRow, rowindex, _rawIndex, _rowsIndex] = mIndex[mComposition->RawGlyphs()[index - 1].get()];
				row = nRow;
				rawIndex = _rawIndex;
				rowsIndex = _rowsIndex;
			}



			for (auto it = begin; it != end; it++) {
				row->Add(std::make_unique<GlyphDecoratorNonOwner>(it->get()), rawIndex);
				mIndex[it->get()] = { (RowGlyph*)&row, rawIndex, index, rowsIndex};
				index++;
			}
		}


		if (row->children() > 1) {
			auto width = mComposition->Bounds().x;
			//last glyph in the row 
			auto [nRow, rowindex, endRawIndex, rowsIndex] = mIndex[(--row->end()).operator*().get()];
			//end iterator over the raw glyphs
			auto endRaw = mComposition->RawGlyphs().begin() + endRawIndex + 1;
			//start with no glyphs
			auto startRaw = endRaw;
			for (auto it = --row->end(); row->Bounds().x > width; --row) {
				//don't need to remove from mIndex, as it will get overwritten in ElementAdd
				row->Remove(it);

				//add this glyph to be inserted in the next row
				--startRaw;
			}
			ElementAdded(startRaw, endRaw);
		}
	}

	virtual void ElementRemoved(Iterator begin, Iterator end) {
		//remove all elements
		size_t eraseRowEnd;
		auto& column = mComposition->SelfGlyph();
		{
			auto beginT = begin;
			auto amountToDelete = (size_t) (end - begin);

			auto [row, rowindex, _rawIndex, eraseRowStart] = mIndex[begin->get()];
			eraseRowEnd = eraseRowStart;

			while (beginT != end) {
				auto [row, rowindex, _rawIndex, rowsIndex] = mIndex[begin->get()];
				auto elemsAtStart = row->children();

				if (elemsAtStart > amountToDelete) {
					//remove from row
					auto b = row->begin() + rowindex;
					auto rowEnd = row->end();
					for (auto it = b; it != rowEnd; it++) {
						row->Remove(it);
					}
				}
				else {
					eraseRowEnd++;
				}

				//remove from index
				for (auto it = beginT; it != end; it++) {
					mIndex.erase(mIndex.find(it->get()));
				}

				beginT = beginT + (elemsAtStart - row->children());
			}

			column->Remove(column->begin() + eraseRowStart, column->begin() + eraseRowEnd);
		}


		if (column->children() != eraseRowEnd) {
			auto& rowIt = column->begin() + eraseRowEnd);
			int width = mComposition->Bounds().x;
			auto rowEnd = column->end();
			auto rowsIndex = eraseRowEnd;
			for (auto rowIt = column->begin() + eraseRowEnd
				; (*rowIt)->Bounds().x < width &&
				  rowIt + 1 != rowEnd
				; rowIt++, rowsIndex++) {
				auto& row = *rowIt;
				auto& nextRow = *(rowIt + 1);
				
				auto it = nextRow->begin();
				auto itEnd = nextRow->end();
				for (; it != itEnd && row->Bounds().x < width; it++) {
					row->Add(std::move(*it));
					//mIndex[(*it).get()] = { (RowGlyph*)&row, rawIndex, index, rowsIndex };
				}
				nextRow->Remove(nextRow->begin(), it);
			}
		}
	}
};



