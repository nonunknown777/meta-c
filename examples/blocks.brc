package BLOCKS_DEMO

block global = 256MB
block game = 64MB
block assets = 128MB
block temp = 8MB

struct Player {
    int hp
    int ammo
    String name

    fn Player(int h, int a, String n) {
        hp = h
        ammo = a
        name = n
    }

    fn take_damage(int dmg) {
        hp -= dmg
        if hp < 0 {
            hp = 0
        }
    }
}

struct Enemy {
    int hp
    int damage

    fn Enemy(int h, int d) {
        hp = h
        damage = d
    }

    fn attack(Player target) {
        target.take_damage(damage)
    }
}

fn main() {
    Player p = Player(100, 30, "Felipe") @game
    Enemy e = Enemy(50, 10) @game

   

    e.attack(p)

    int game_iterations = 0 @game

    while p.hp > 0 {
        e.attack(p)
        game_iterations += 1
    }

    temp.reset()
    game.reset()
    global.reset()
}
