.pragma library

const assets = {
    bleeding: "droplet",
    blinded: "eye-off",
    confused: "shuffle",
    cowering: "despair",
    dazed: "knocked-out-stars",
    dazzled: "sun",
    dead: "skull",
    deafened: "ear-off",
    disabled: "circle-minus",
    dying: "heart-pulse",
    entangled: "fishing-net",
    fascinated: "sparkles",
    fear: "ghost",
    flatFooted: "footprints",
    grappled: "link-2",
    helpless: "shield-off",
    incorporeal: "wind",
    invisible: "person-standing",
    nauseated: "face-vomiting",
    negativeLevels: "trending-down",
    paralyzed: "zap",
    petrified: "mountain",
    pinned: "lock-keyhole",
    prone: "despair",
    sickened: "pill",
    stable: "heart-plus",
    staggered: "route",
    stunned: "punch",
    unconscious: "sleep",
    fatigue: "battery-low"
};

function source(conditionId) {
    const name = assets[conditionId];
    return name ? "condition-icons/" + name + ".svg" : "";
}
