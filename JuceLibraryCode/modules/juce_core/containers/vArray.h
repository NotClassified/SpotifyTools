#pragma once
#include <vector>

//contains a std::vector
template<class Type>
class vArray
{
public:
	vArray() {}

	using Iterator = typename std::vector<Type>::iterator;

	std::vector<Type> data;

	Type& operator[](size_t i) { return data.operator[](i); }
	Type& getLast() { return *(data.end() - 1); }

	size_t size() const { return data.size(); }
	size_t lastIndex() const { return data.size() - 1; };
	bool isEmpty() const { return data.empty(); }
	bool isNotEmpty() const { return !data.empty(); }

	bool contains(const Type& val)
	{
		for (auto& e : data)
		{
			if (e == val)
				return true;
		}
		return false;
	}

	int indexOf(const Type& val)
	{
		for (size_t i = 0; i < size(); i++)
		{
			if (data[i] == val)
				return i;
		}
		return -1;
	}

	Iterator begin() { return data.begin(); }
	Iterator end() { return data.end(); }

	void add(const Type& val) { data.push_back(val); }
	void add(vArray<Type>& arr)
	{
		for (auto& val : arr.data)
			data.push_back(val);
	}
	void addUnique(const Type& val)
	{
		if (!contains(val))
			add(val);
	}

	void removeAt(size_t index) { data.erase(data.begin() + index); }
	void remove(const Type elementToRemove)
	{
		for (Iterator i = data.begin(); i != data.end(); i++)
		{
			if (*i == elementToRemove)
			{
				data.erase(i);
				break;
			}
		}
	}
	void removeDuplicates(std::function<bool(Type& a, Type& b)> duplicateCondition)
	{
		for (size_t i = 0; i < data.size(); i++)
		{
			for (size_t j = i + 1; j < data.size(); j++)
			{
				if (duplicateCondition(data[i], data[j]))
				{
					removeAt(j);
					j--;
				}
			}
		}
	}
	void removeIf(std::function<bool(Type& a)> condition)
	{
		for (size_t i = 0; i < data.size(); i++)
		{
			if (condition(data[i]))
			{
				removeAt(i);
				i--;
			}
		}
	}

	void preallocate(size_t newCapacity) { data.reserve(newCapacity); }
	void clear() { data.clear(); }

	Type* search(std::function<bool(Type& a)> predicate)
	{
		for (auto& element : data)
		{
			if (predicate(element))
				return &element;
		}
		return nullptr;
	}

	void sort(std::function<bool(Type& a, Type& b)> swapPredicate)
	{
		int n = size();
		bool swapped;

		for (int i = 0; i < n - 1; i++)
		{
			swapped = false;
			for (int j = 0; j < n - i - 1; j++)
			{
				if (swapPredicate(data[j], data[j + 1]))
				{
					std::swap(data[j], data[j + 1]);
					swapped = true;
				}
			}

			// If no two elements were swapped, then break
			if (!swapped)
				break;
		}
	}
};