#pragma once
#include <cstdint>
#include <stdexcept>

template<typename T>
class GameBoardMatrix
{
public:
	GameBoardMatrix(uint32_t rows, uint32_t cols, const T& fill) : m_rows(0), m_cols(0), m_data(nullptr)
	{
		resize(rows,cols,fill);
	}

	GameBoardMatrix(const GameBoardMatrix<T>& r) : m_rows(r.m_rows), m_cols(r.m_cols), m_data(nullptr)
	{
		*this = r;
	}

	virtual ~GameBoardMatrix()
	{
		for (uint32_t row = 0; row < m_rows; row++)
		{
			delete[] m_data[row];
		}

		delete[] m_data;
	}

public:
	GameBoardMatrix<T>& operator=(const GameBoardMatrix<T>& right)
	{
		// clear the old values
		for (uint32_t row = 0; row < m_rows; row++)
		{
			delete[] m_data[row];
		}

		delete[] m_data;
		m_rows = 0;
		m_cols = 0;
		m_data = nullptr;

		// now reconstruct with the new
		resize(right.m_rows, right.m_cols, right.item(0, 0));

		for (uint32_t row = 0; row < m_rows; row++)
		{
			for (uint32_t col = 0; col < m_cols; col++)
			{
				m_data[row][col] = right.m_data[row][col];
			}
		}

		return *this;
	}

	bool operator==(const GameBoardMatrix<T>& right) const
	{
		bool areIdentical(true);

		for (uint32_t row = 0; row < m_rows; row++)
		{
			for (uint32_t col = 0; col < m_cols; col++)
			{
				areIdentical &= (m_data[row][col] == right.m_data[row][col]);
			}
		}

		return areIdentical;
	}

public:
	void resize(uint32_t rows, uint32_t cols, const T& fill)
	{
		// a grid is an array of rows, where each row is an array of T
		// a zero-row or zero-column GameBoardMatrix has a null grid
		// TODO - make this exception-safe - new could throw here and that would cause a memory leak

		T** new_grid = nullptr;

		if (rows && cols)
		{
			new_grid = new T*[rows];
			for (uint32_t row = 0; row < rows; row++)
			{
				new_grid[row] = new T[cols];

				// copy old items to the new grid but only within the bounds of the intersection of the old and new grids
				// fill the rest of the grid with the initial value
				for (uint32_t col = 0; col < cols; col++)
				{
					if (row < m_rows && col < m_cols)
					{
						new_grid[row][col] = m_data[row][col];
					}
					else
					{
						new_grid[row][col] = fill;
					}
				}
			}
		}

		// destroy the old grid
		for (uint32_t row = 0; row < m_rows; row++)
		{
			delete[] m_data[row];
		}

		delete[] m_data;

		// move the new data into the GameBoardMatrix
		m_data = new_grid;
		m_rows = rows;
		m_cols = cols;
	}

public:
	uint32_t rows() const
	{
		return m_rows;
	}
	uint32_t columns() const
	{
		return m_cols;
	}

	void erase(const T& fill)
	{
		for (uint32_t row = 0; row < m_rows; row++)
			for (uint32_t col = 0; col < m_cols; col++)
				insert(row,col,fill);
	}

	void erase(uint32_t row, uint32_t col, const T& fill)
	{
		insert(row,col,fill);
	}

	void insert(uint32_t row, uint32_t col, const T& element)
	{
		if (row >= m_rows) throw std::out_of_range("GameBoardMatrix::insert row");
		if (col >= m_cols) throw std::out_of_range("GameBoardMatrix::insert col");
		m_data[row][col] = element;
	}

	T& item(uint32_t row, uint32_t col)
	{
		if (row >= m_rows) throw std::out_of_range("GameBoardMatrix::item row");
		if (col >= m_cols) throw std::out_of_range("GameBoardMatrix::item col");
		return m_data[row][col];
	}

	const T& item(uint32_t row, uint32_t col) const
	{
		if (row >= m_rows) throw std::out_of_range("GameBoardMatrix::item row");
		if (col >= m_cols) throw std::out_of_range("GameBoardMatrix::item col");
		return m_data[row][col];
	}

	T& operator()(uint32_t row, uint32_t col)
	{
		if (row >= m_rows) throw std::out_of_range("GameBoardMatrix::operator() row");
		if (col >= m_cols) throw std::out_of_range("GameBoardMatrix::operator() col");
		return m_data[row][col];
	}

	const T& operator()(uint32_t row, uint32_t col) const 
	{
		if (row >= m_rows) throw std::out_of_range("GameBoardMatrix::operator() row");
		if (col >= m_cols) throw std::out_of_range("GameBoardMatrix::operator() col");
		return m_data[row][col];
	}

	void fill(const T& item)
	{
		erase(item);
	}

	void fill_column(uint32_t col, const T& item)
	{
		if (col >= m_cols) throw std::out_of_range("GameBoardMatrix::fill_column");
		for (uint32_t row = 0; row < m_rows; row++)
		{
			insert(row, col, item);
		}
	}

	void fill_row(uint32_t row, const T& item)
	{
		if (row >= m_rows) throw std::out_of_range("GameBoardMatrix::fill_row");
		for (uint32_t col = 0; col < m_cols; col++)
		{
			insert(row, col, item);
		}
	}

	void fill_leading_diagonal(const T& item)
	{
		for (uint32_t i = 0; i < m_cols && i < m_rows; i++)
		{
			insert(i, i, item);
		}
	}

	void fill_trailing_diagonal(const T& item) 
	{
		for (uint32_t i = 0; i < m_cols && i < m_rows; i++)
		{
			insert(i, m_cols-i-1, item);
		}
	}

	void make_identity(const T& one, const T& zero)
	{
		fill(zero);
		fill_leading_diagonal(one);
	}

	void transpose()
	{
		// no gain in manipulating this, since building a new GameBoardMatrix is no less efficient
		GameBoardMatrix<T> transposed(columns(), rows());
		for (uint32_t row = 0; row < rows(); row++)
		{
			for (uint32_t col = 0; col < columns(); col++)
			{
				transposed.insert(col, row, item(row,col));
			}
		}

		// TODO - avoid an extra copy by swapping the member data here
		*this = transposed;
	} 

private:
	uint32_t m_rows;
	uint32_t m_cols;
	T** m_data;
};
