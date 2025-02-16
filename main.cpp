#include "Component.h"

#include <ChunkArray.h>
#include <iostream>
#include <chrono>
 
class pawn_t
{
public:
	int id;
	vec3 position;
	vec4 rotation; 
};

int main() 
{
    using t = archetype<meta_list<entity, vec3, vec4>, 
        meta_list<std::array<float,128>, std::array<float, 128>, std::array<float, 128>>>;
    t pawn{};
  
    std::vector< pawn_t> pawns;
    std::vector< pawn_t*> pawns2;
    for (auto i = 0; i < 10000000; ++i)
    {
		auto& e = pawn.push_back();
        e.id = i + 1;

        pawns.push_back( pawn_t{ .id = i + 1, });
        pawns2.push_back(new pawn_t{ .id = i + 1, });
    }
   
    pawn.erase(entity{ .id = 7 });
    
	query<t> q(&pawn);
    auto start = std::chrono::high_resolution_clock::now();
	q.for_each_entity_chunk([](vec3& v) 
        {
            v.x += 1.0f;
		});
    auto end = std::chrono::high_resolution_clock::now();

    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "ecs archetype执行时间: " << duration.count() << " 微秒\n";



	
     start = std::chrono::high_resolution_clock::now();
     for (auto& p : pawns)
     {
		 p.position.x += 1.0f;
     }
     end = std::chrono::high_resolution_clock::now();

    // 计算持续时间
     duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "std::vector< pawn_t>执行时间: " << duration.count() << " 微秒\n";


    start = std::chrono::high_resolution_clock::now();
    for (auto& p : pawns2)
    {
        p->position.x += 1.0f;
    }
    end = std::chrono::high_resolution_clock::now();

    // 计算持续时间
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "std::vector< pawn_t*>执行时间: " << duration.count() << " 微秒\n";

    return 0;
}
