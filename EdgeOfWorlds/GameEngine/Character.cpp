#include "stdafx.h"
#include "Character.h"
#include <assert.h>
#include "Elements.h"
/**
  * 
  */

Character::Character(pugi::xml_node& node) :
	m_name(node.attribute("name").as_string()),
	m_level(node.attribute("level").as_int()),
	m_stats{node.child("Stats")},
	m_baseStats{ node.child("Stats") },
	m_actualHP(node.child("Stats").attribute("HP").as_int()),
	m_weaknesses{ W_NONE, W_NONE, W_NONE, W_NONE, W_NONE, W_NONE, W_NONE, W_NONE },
	m_baseWeaknesses{ W_NONE, W_NONE, W_NONE, W_NONE, W_NONE, W_NONE, W_NONE, W_NONE }
{
	for (auto & a : node.child("Weaknesses").attributes())
	{
		m_weaknesses[elementFromString(a.name())] = weaknessFromString(a.as_string());
	}
	int i(0);
	for (auto & n : node.child("Skills").children())
	{
		m_skills[i] = std::make_unique<Skill>(n);
		i++;
	}
	pugi::xml_node n = node.child("Equipement");
	addEquipement(n);

	updateStats();

}

/*
Character::Character(pugi::xml_node& node, int const level) :
	m_name(node.attribute("name").as_string()),
	m_level(level),
	m_stats{ node.child("BaseStats") },
	m_baseStats{ node.child("BaseStats") }
{

	/*
	 * TODO : - define stats by level
	 *        - add random skills by level + xml
	 *		  - add random equipement
	 *



	m_skills[0] = std::make_unique<Skill>(node.child("Skills"));
	int i(0);
	for (auto n : node.child("Skills").children())
	{
		m_skills[i] = std::make_unique<Skill>(n);
		i++;
	}

}*/

void Character::getHit(int const attPower)
{
	if (m_dead)
	{
		return;
	}

	m_actualHP -= attPower;
	if (m_actualHP > m_baseStats.HP)
	{
		m_actualHP = m_baseStats.HP;
	}
	else if (m_actualHP <= 0)
	{
		onDeath();
	}
	else
	{
		onHit();
	}
}

void Character::hit(int const& skillID, Character * target)
{
	// Pre-condition : skills[skillID] != NULL
	assert(m_skills[skillID] != nullptr);

	int lostHP = 0;

	if (m_skills[skillID]->isMagical() && m_stats.power != 0){
		lostHP = m_skills[skillID]->getPureDamage() +
			(m_stats.power * m_stats.power * m_skills[skillID]->getModifier()) /
			target->m_stats.resist /
			100;	// les 100 du modifier en pourcentage
	}
	else if (m_skills[skillID]->isMagical()) {
		lostHP = 0;
	}
	else {
		lostHP = m_skills[skillID]->getPureDamage() +
			(m_stats.strength * m_stats.strength * m_skills[skillID]->getModifier()) /
			target->m_stats.defense /
			100;	// les 100 du modifier en pourcentage
	}

	for (auto & s : m_skills[skillID]->getStatuts())
	{
		target->m_activeStatuts.emplace_back(s);
	}
	int weaknessModifier = (getModifierFromWeakness(target->m_weaknesses[m_skills[skillID]->getElement()]));
	int increaseModifier = target->getIncreaseness(m_skills[skillID]->getElement());
	lostHP = lostHP * (m_skills[skillID]->isHealer() ? -1 : 1) * weaknessModifier * increaseModifier / 10000; // % %
	m_skills[skillID]->updateCharges();

	target->getHit(lostHP);
}

void Character::setSkill(int i, pugi::xml_node & node) // 2 <= i < 6
{
	if (!m_skills[i].operator bool())
	{
		m_skills[i].reset();
	}
	m_skills[i] = std::make_unique<Skill>(node);
}

void Character::updateStats()
{

	m_stats = m_baseStats;
	for (auto & i : m_activeStatuts)
	{
		if (!(i.getEffect() == SOFTHEAL || i.getEffect() == POISON))
		{
			applyUniqueStatut(i.getEffect());
		}
	}
	for (auto & i : m_equipements)
	{
		if (i.operator bool())
			m_stats += i->getBonuses();
	}
	m_actualHP = (m_stats.HP < m_actualHP) ? m_stats.HP : m_actualHP;

	
	m_weaknesses[E_DARK] = m_baseWeaknesses[E_DARK];
	m_weaknesses[E_EARTH] = m_baseWeaknesses[E_EARTH];
	m_weaknesses[E_FIRE] = m_baseWeaknesses[E_FIRE];
	m_weaknesses[E_HOLY] = m_baseWeaknesses[E_HOLY];
	m_weaknesses[E_NONE] = m_baseWeaknesses[E_NONE];
	m_weaknesses[E_THUNDER] = m_baseWeaknesses[E_THUNDER];
	m_weaknesses[E_WATER] = m_baseWeaknesses[E_WATER];
	m_weaknesses[E_WIND] = m_baseWeaknesses[E_WIND];
	for (auto & j : m_equipements)
	{
		if (j != nullptr)
		{
			m_weaknesses[E_DARK] = combineWeakness(m_weaknesses[E_DARK], j->getWeakness(E_DARK));
			m_weaknesses[E_EARTH] = combineWeakness(m_weaknesses[E_EARTH], j->getWeakness(E_EARTH));
			m_weaknesses[E_FIRE] = combineWeakness(m_weaknesses[E_FIRE], j->getWeakness(E_FIRE));
			m_weaknesses[E_HOLY] = combineWeakness(m_weaknesses[E_HOLY], j->getWeakness(E_HOLY));
			m_weaknesses[E_NONE] = combineWeakness(m_weaknesses[E_NONE], j->getWeakness(E_NONE));
			m_weaknesses[E_THUNDER] = combineWeakness(m_weaknesses[E_THUNDER], j->getWeakness(E_THUNDER));
			m_weaknesses[E_WATER] = combineWeakness(m_weaknesses[E_WATER], j->getWeakness(E_WATER));
			m_weaknesses[E_WIND] = combineWeakness(m_weaknesses[E_WIND], j->getWeakness(E_WIND));

		}
	}
	

}

void Character::addEquipement(pugi::xml_node & node)
{
	Equipement::EquipType type = Equipement::equipTypeFromString(node.attribute("equipType").as_string());
	m_equipements[type].reset(new Equipement(node));
	updateStats();
}

void Character::addStatut(Statut s)
{
	m_activeStatuts.push_back(s);
}

void Character::onDeath()
{
	m_dead = true;
	die();
}

void Character::onTurnEnd()
{
	int j = 0;
	for (auto & i : m_activeStatuts)
	{
		if (i.getEffect() == SOFTHEAL || i.getEffect() == POISON)
		{
			applyUniqueStatut(j);
		}
		i.update();
		if (i.isExpired())
		{
			m_activeStatuts.erase(m_activeStatuts.begin()+j);
		}
		j++;
	}
}

Stats const& Character::getStats() const
  {
	  return m_stats;
  }

  Stats const& Character::getBaseStats() const
  {
	  return m_baseStats;
  }

  std::string const& Character::getName() const
  {
	  return m_name;
  }

  Skill const * Character::getSkill(int i) const // pr�cond. 0 <= i < 6
  {
	  // les skills 1 & 2 sont d�finis par l'arme si elle existe
	  if (i < 2)
	  {
		  if (m_equipements[Equipement::WEAPON].operator bool())
		  {
			  if (i == 0)
				  return m_equipements[Equipement::WEAPON]->useAttack();
			  else
				  return m_equipements[Equipement::WEAPON]->useSkill();
		  }
	  }
	 
	  return m_skills[i].get();
  }

  Equipement const * Character::getEquipement(Equipement::EquipType t) const
  {
	  return m_equipements[t].get();
  }

  int const & Character::getHP() const
  {
	  return m_actualHP;
  }

  std::vector<Statut> const & Character::getActiveStatuts() const
  {
	  return m_activeStatuts;
  }

  int const Character::getIncreaseness(Element el) const
  {
	  int coef = 1;
	  for (auto & e : m_equipements) {
		  if (e)
			 coef *= e->getIncreaseness(el);
	  }
	  return coef;
  }

  void Character::applyUniqueStatut(int i)
  {
	  int pow = m_activeStatuts[i].getPower();
	  switch (m_activeStatuts[i].getEffect()) {
	  case POISON:
		  poison(pow);
		  break;
	  case WEAK:
		  weak(pow);
		  break;
	  case BRITTLE:
		  brittle(pow);
		  break;
	  case MUTE:
		  mute(pow);
		  break;
	  case SLOW:
		  slow(pow);
		  break;
	  case SOFTHEAL:
		  softHeal(pow);
		  break;
	  case STRONG:
		  strong(pow);
		  break;
	  case UNBREAKABLE:
		  unbreakable(pow);
		  break;
	  case QUICK:
		  quick(pow);
		  break;
	  case FREEZE:
		  freeze(pow);
		  break;
	  default:
		  effect(pow);
		  break;
	  }
  }

  void Character::poison(int p)
  {
	  m_actualHP -= p * m_stats.HP / 100;
  }

  void Character::weak(int p)
  {
	  m_stats.strength -= p * m_stats.strength / 100;
  }

  void Character::brittle(int p)
  {
	  m_stats.defense -= p * m_stats.defense / 100;
  }

  void Character::mute(int p)
  {
	  m_stats.power = 0;
  }

  void Character::slow(int p)
  {
	  m_stats.speed -= p * m_stats.speed / 100;
  }

  void Character::softHeal(int p)
  {
	  m_actualHP += p * m_stats.HP / 100;
  }

  void Character::strong(int p)
  {
	  m_stats.strength += p * m_stats.strength / 100;
  }

  void Character::unbreakable(int p)
  {
	  m_stats.defense += p * m_stats.defense / 100;
  }

  void Character::quick(int p)
  {
	  m_stats.speed += p * m_stats.speed / 100;
  }

  void Character::freeze(int)
  {
	  m_stats.speed = 0;
  }

  void Character::draw(sf::RenderTarget & t)
  {
	  update();
	  t.draw(m_sprite);
  }

  void Character::drawGohst(sf::RenderTarget & t)
  {
	  update();
	  t.draw(m_spriteGohst);
  }

  void Character::setPosition(sf::Vector2f v)
  {
	  m_sprite.setPosition(v.x, v.y /*- ( m_sprite.getScale().y /3)*/);
	  m_spriteGohst.setPosition(v.x, v.y /*- ( m_sprite.getScale().y /3)*/);
  }


  void Character::setTexture()
  {
	  if (m_team == BLUE)
	  {
		  m_texture.loadFromFile("../Assets/sprites/blueSprite.png");
	  }
	  else if (m_team == RED)
	  {
		  m_texture.loadFromFile("../Assets/sprites/redSprite.png");
	  }
	  m_spriteWidth = m_texture.getSize().x / 12;
	  m_spriteHeight = m_texture.getSize().y / 8;
	  m_sprite.setTexture(m_texture);
	  sf::IntRect bonds(0, 0, m_spriteWidth, m_spriteHeight);
	  m_sprite.setTextureRect(bonds);
	  m_sprite.setOrigin(0, m_spriteHeight / 3);

	  if (m_team == BLUE)
	  {
		  m_textureGohst.loadFromFile("../Assets/sprites/blueSpriteShadow.png");
	  }
	  else if (m_team == RED)
	  {
		  m_textureGohst.loadFromFile("../Assets/sprites/redSpriteShadow.png");
	  }
	  m_spriteGohst.setTexture(m_textureGohst);
	  m_spriteGohst.setTextureRect(bonds);
	  m_spriteGohst.setOrigin(0, m_spriteHeight / 3);
  }

  void Character::update()
  {
	  // if dead
	  if (m_animationLine >= 12 && m_animationIterator == 5)
	  {
		  return;
	  }
	  //  marche
	  else if (m_animationLine == 0)
	  {
		  setPosition(sf::Vector2f{
			  m_sprite.getPosition().x - m_sprite.getTextureRect().width /10,  // 5 car 6 animations, 2 car ...
			  m_sprite.getPosition().y + m_sprite.getTextureRect().height /30  // 5 car 6 animations, 3 car sprite, 2 car ...
			  });
		  m_animationIterator = (m_animationIterator + 1) % 6;
		  if (m_animationIterator == 0)
		  {
			  setPosition(sf::Vector2f{
				  m_sprite.getPosition().x - 4,
				  m_sprite.getPosition().y + 2 
				  });
			  m_animationLine = 8;
		  }
	  }
	  else if (m_animationLine == 1)
	  {
		  setPosition(sf::Vector2f{
			  m_sprite.getPosition().x + m_sprite.getTextureRect().width / 10,
			  m_sprite.getPosition().y + m_sprite.getTextureRect().height / 30
			  });
		  m_animationIterator = (m_animationIterator + 1) % 6;
		  if (m_animationIterator == 0)
		  {
			  setPosition(sf::Vector2f{
				  m_sprite.getPosition().x + 4,
				  m_sprite.getPosition().y + 2
				  });
			  m_animationLine = 9;
		  }
	  }
	  else if (m_animationLine == 2)
	  {
		  setPosition(sf::Vector2f{
			  m_sprite.getPosition().x + m_sprite.getTextureRect().width / 10,
			  m_sprite.getPosition().y - m_sprite.getTextureRect().height / 30
			  });
		  m_animationIterator = (m_animationIterator + 1) % 6;
		  if (m_animationIterator == 0)
		  {
			  setPosition(sf::Vector2f{
				  m_sprite.getPosition().x + 4,
				  m_sprite.getPosition().y - 2
				  });
			  m_animationLine = 10;
		  }
	  }
	  else if (m_animationLine == 3)
	  {
		  setPosition(sf::Vector2f{
			  m_sprite.getPosition().x - m_sprite.getTextureRect().width / 10,
			  m_sprite.getPosition().y - m_sprite.getTextureRect().height / 30
			  });
		  m_animationIterator = (m_animationIterator + 1) % 6;
		  if (m_animationIterator == 0)
		  {
			  setPosition(sf::Vector2f{
				  m_sprite.getPosition().x - 4,
				  m_sprite.getPosition().y - 2
				  });
			  m_animationLine = 11;
		  }
	  }
	  else
	  {
		  m_animationIterator = (m_animationIterator + 1) % 6;

	  }
	  if (m_animationLine >= 4 && m_animationIterator == 0)
		  m_animationLine = m_animationLine % 4 + 8;
	  // animations statiques
	  sf::IntRect bonds((m_animationIterator + (m_animationLine / 8) * 6)* m_spriteWidth,
		  (m_animationLine % 8) * m_spriteHeight,
		  m_spriteWidth,
		  m_spriteHeight);
	  m_sprite.setTextureRect(bonds);
	  m_spriteGohst.setTextureRect(bonds);
	  // post - frapper


  }

  void Character::move(Direction d)
  {
	  m_animationIterator = 1;
	  switch (d)
	  {
	  case UP:
		  m_animationLine = 2;
		  break;
	  case DOWN:
		  m_animationLine = 0;
		  break;
	  case LEFT:
		  m_animationLine = 3;
		  break;
	  case RIGHT:
		  m_animationLine = 1;
		  break;
	  }
  }

  void Character::HitAnimation(Direction d)
  {
	  m_animationIterator = 1;
	  switch (d)
	  {
	  case UP:
		  m_animationLine = 6;
		  break;
	  case DOWN:
		  m_animationLine = 4;
		  break;
	  case LEFT:
		  m_animationLine = 7;
		  break;
	  case RIGHT:
		  m_animationLine = 5;
		  break;
	  }
  }

  void Character::die()
  {
	  m_animationIterator = 1;
	  m_animationLine += 4;
  }

  void Character::changeDirection(Direction d)
  {
	  switch (d)
	  {
	  case UP:
		  m_animationLine = 10;
		  break;
	  case DOWN:
		  m_animationLine = 8;
		  break;
	  case LEFT:
		  m_animationLine = 11;
		  break;
	  case RIGHT:
		  m_animationLine = 9;
		  break;
	  }
  }
