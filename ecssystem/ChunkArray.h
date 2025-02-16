#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cstddef>
#include <cassert> 

#include <type_traits>
#include <utility>
#include <array>
#include <list>
#include <vector>

class fragment_descriptor
{
public:
    int size : 28;
    int align : 4;
};


class entity
{
public:

	auto operator<=>(const entity& other) const
	{
		return id <=> other.id;
	}

	auto operator==(const entity& other) const
	{
		return id == other.id;
	}

	void operator=( entity other) 
	{
		id = other.id;
	}

	int id; 


};

class vec3
{
public:
	double x, y, z;
};

class vec4
{
public:
    float x, y, z ,w;
};

template<class..._Types>
class meta_list;

template<>
class meta_list<>
{
public:
    using types = std::tuple<>;

    static constexpr auto count = 0;

    static constexpr auto size = 0;
};

template <typename T, size_t N>
struct ToArray {
    using type = T[N]; // 将 T 转换为 T[N]
};

template <typename Tuple, size_t N, typename = void>
struct TupleToArrays;
 
template <size_t N>
struct TupleToArrays<std::tuple<>, N> {
    using type = std::tuple<>; // 返回空元组
};
 
template <typename Head, typename... Tail, size_t N>
struct TupleToArrays<std::tuple<Head, Tail...>, N> {
    using HeadArray = typename ToArray<Head, N>::type; // 将 Head 转换为 Head[N]
    using TailArrays = typename TupleToArrays<std::tuple<Tail...>, N>::type; // 递归处理剩余类型
    using type = decltype(std::tuple_cat(
        std::declval<std::tuple<HeadArray>>(), // 将 Head[N] 封装为元组
        std::declval<TailArrays>()            // 递归结果
    ));
};

template<class..._Types>
class meta_list
{
public:
    using types = std::tuple<_Types...>;

    static constexpr auto count = sizeof...(_Types);

    static constexpr auto size = (sizeof(_Types) + ...);

   
};
 

// 主模板声明（前向声明）
template < size_t Offset,size_t Size, size_t N, class OffsetList, class _Types ,size_t Index>
struct _Archetype_layout;

// 终止条件：处理完所有类型，返回最终总大小和偏移量元组
template <
    size_t Offset,
    size_t Size,
    size_t N,
    class OffsetList,
    class _Types
>
struct _Archetype_layout<Offset, Size,  N, OffsetList, _Types,Size> {
    static constexpr size_t value = Offset;// ((Offset + MaxAlign - 1) / MaxAlign)* MaxAlign; // 最终总大小
    using offsets = OffsetList; // 最终的偏移量元组
};

// 递归处理：计算当前类型偏移量，更新元组
template <
    size_t Offset,
    size_t Size,
    size_t N, 
    class OffsetList,
    class Types,
	size_t Index
>
struct _Archetype_layout 
{ 
	using _Ty = std::tuple_element_t<Index, Types>;
    // 计算对齐后的偏移量
    static constexpr size_t align = alignof(_Ty);
    static constexpr size_t current_offset = ((Offset + align - 1) / align) * align;
    static constexpr size_t new_offset = current_offset + sizeof(_Ty) * N;
    
    // 将当前偏移量添加到元组中
    using NewOffset = std::integral_constant<size_t, current_offset>;
    using NewOffsetList = decltype(std::tuple_cat(OffsetList{}, std::make_tuple(NewOffset{})));

    // 递归处理下一个元素
    using Next = _Archetype_layout <
        new_offset,
        Size,
        N,
        NewOffsetList,
        Types,
        Index+1
    >;
	 
    // 传递最终结果
    static constexpr size_t value = Next::value;
    using offsets = typename Next::offsets;
};

// 编译期元组到数组的转换函数
template <typename Tuple, size_t... Indices>
constexpr auto convert_offsets_to_array_impl(const Tuple&, std::index_sequence<Indices...>)
{
    return std::array<int, sizeof...(Indices)>{ static_cast<int>(std::get<Indices>(Tuple{}).value)... };
}
 
// 封装函数：将偏移量元组转换为编译期数组
template <typename OffsetTuple>
constexpr auto convert_offsets_to_array() {
    return convert_offsets_to_array_impl(
        OffsetTuple{},
        std::make_index_sequence<std::tuple_size_v<OffsetTuple>>{}
    );
}
 
 
template <class fragment,
            class cache = meta_list<>,
            class shared = meta_list<>,
            size_t capcity = 128*1024 >
class archetype
{
public:
	
    static constexpr auto chunk_fragment_capcity = capcity - sizeof(typename cache::types);
	static constexpr auto chunk_entity_max_count = chunk_fragment_capcity / sizeof(typename fragment::types);
	
    static constexpr auto chunk_entity_count = sizeof(
        std::tuple<typename TupleToArrays<meta_list<entity, vec3, vec4>::types, chunk_entity_max_count>::type , typename cache::types>
        ) > chunk_fragment_capcity ? chunk_entity_max_count - 1 : chunk_entity_max_count;
    
	using fragment_container = typename TupleToArrays<meta_list<entity, vec3, vec4>::types, chunk_entity_count>::type;
	using cache_container = typename cache::types;
	using shared_container = typename shared::types;

	template <typename T, typename... Ts>
	static constexpr auto& get_by_type(std::tuple<Ts...>& t) {
		constexpr size_t index = []() {
			size_t idx = 0;
			bool found = ((std::is_same_v<T, Ts> ? true : (++idx, false)) || ...);
			return found ? idx : throw "Type not found!";
			}();
		return std::get<index>(t);
	}
	 
	class chunk_type  
    {
    public:
        
        fragment_container fragments;
        cache_container caches;

        friend class archetype;
        
    };

 
	auto size() const
	{
		return this->_Size;
	}

	auto capacity() const
	{
		return _Capacity;
	}

	auto empty() const
	{
		return _Size == 0;
	}

	auto& push_back()
	{ 
		if (_Size >= _Capacity)
		{
			chunks.emplace_back(std::make_unique<chunk_type>());
			_Capacity += chunk_entity_count;
		}
		 
		_Size++;
		auto& ret = at<entity>(_Size - 1);
		 
		return ret;
	}

	auto pop_back()
	{
		if (_Size > 0)
		{
#if _DEBUG
			back().id = 0;
#endif
			_Size--;
		}

	}

	auto& back()
	{
		if (_Size > 0)
		{
			return at<entity>(_Size - 1);
		}
		else
		{
			throw std::out_of_range("Deque index out of range");
		}
	}

	auto& front()
	{
		if (_Size > 0)
		{
			return at<entity>(0);
		}
		else
		{
			throw std::out_of_range("Deque index out of range");
		}
	}

	auto clear()
	{
		_Size = 0;
	}

	auto begin()
	{
		return at<entity>(0);
	}

	auto end()
	{
		return at<entity>(_Size);
	}

	auto erase(entity e)
	{
		if (e == back())
		{
			pop_back();
		}
		else
		{
			for (auto it = chunks.begin(); it != chunks.end(); it++)
			{
				auto arr = get_by_type<entity[chunk_entity_count]>((*it)->fragments);

				for (int i = 0; i < chunk_entity_count; ++i)
				{
					if (arr [i] == e)
					{
						arr[i] = back();
						pop_back();
						break;
					}
				}
			}
			 
		}
	}

	template<class _Ty>
	auto& at(size_t n) 
	{
		if (n >= _Size ) throw std::out_of_range("Deque index out of range");
		 
		// 计算块索引和块内偏移
		size_t block_index = (n / chunk_entity_count);
		size_t element_index = n % chunk_entity_count;

		return get_by_type<_Ty[chunk_entity_count]>(chunks[block_index]->fragments)[element_index];
	}
protected:
	
	size_t _Capacity = 0; 
	size_t _Size = 0; 

	std::vector<std::unique_ptr<chunk_type >> chunks; 
    shared_container shared_chunks;
     
	template <typename U>
	friend class query;
};
 
template<class _Archetype>	
class query
{
public:
	query(_Archetype* arche) : _Arche(arche) {}

	template<class _Fty>
	void for_each_entity_chunk (_Fty _Func)
	{
		for (int i = 0; i < _Arche->_Size; ++i)
		{
			_Func(_Arche->at<vec3>(i));
		}
	}

	_Archetype* _Arche;
};

class execution_context
{
public:
	 
};

class processor
{
public:
	void configure_queries()
	{
		std::cout << "configure" << std::endl;
	}
	void execute()
	{
		std::cout << "run" << std::endl;
	}
};