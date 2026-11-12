#pragma once
#include <string_view>

namespace golarion::embedded
{
    inline constexpr std::string_view RaceDefinitionCatalogJson = R"([
        {"id":"test","name":"Razza di prova","qualities":[],"standardFeatures":[
            {"id":"feature","name":"Tratto","description":"Scelte di prova","choices":[
                {"id":"required","prompt":"Scegli due opzioni","selectionCount":2,"options":[{"id":"a","name":"A"},{"id":"b","name":"B"}]},
                {"id":"optional","prompt":"Scelta non bloccante","selectionCount":1,"blocksAdvancement":false,"options":[{"id":"a","name":"A"}]}
            ]}
        ],"alternateFeatures":[
            {"id":"replacement","name":"Alternativa","description":"Sostituzione","replaces":["feature"]}
        ]}
    ])";
}
