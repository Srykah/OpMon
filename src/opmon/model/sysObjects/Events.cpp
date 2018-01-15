#include "Events.hpp"

#include "../../../utils/defines.hpp"
#include "../../../utils/log.hpp"
#include "../storage/Data.hpp"
#include "../../view/Overworld.hpp"


#pragma GCC diagnostic ignored "-Wreorder"


UNS


namespace OpMon{
  namespace Model{

    using namespace Events;

    Event::~Event(){

    }

    Event::Event(std::vector <sf::Texture> &otherTextures, EventTrigger eventTrigger, sf::Vector2f const &position,
                 int sides, bool passable) :
      otherTextures(otherTextures), eventTrigger(eventTrigger),
      position(sf::Vector2f((position.x + 8) * 32, (position.y + 8) * 32)),
      mapPos(position),
      passable(passable),
      sides(sides),
      currentTexture(otherTextures.begin()){

    }


    namespace Events{

      bool justTP = false;

      sf::Sound doorSound;
      sf::Sound shopdoorSound;

      namespace DoorType{
        std::vector <sf::Texture> NORMAL, SHOP;
      }

      TPEvent::TPEvent(std::vector <sf::Texture> &otherTextures, EventTrigger eventTrigger,
                       sf::Vector2f const &position, sf::Vector2i const &tpPos, string const &map, Side ppDir,
                       int sides, bool passable) :
        Event(otherTextures, eventTrigger, position, sides, passable), tpCoord(tpPos), map(map), ppDir(ppDir){

      }

      DoorEvent::DoorEvent(std::vector <sf::Texture> &doorType, sf::Vector2f const &position, sf::Vector2i const &tpPos,
                           string const &map, EventTrigger eventTrigger, Side ppDir, int sides, bool passable) :
        Event(doorType, eventTrigger, position, sides, passable),
        TPEvent(doorType, eventTrigger, position, tpPos, map, ppDir, sides, passable){
        this->sprite->move(0, -6);
        if(&doorType[0] == &DoorType::SHOP[0]){
          this->position.x -= 4;
          this->doorType = 1;
        }else{
          this->doorType = 0;
        }
      }

      TalkingEvent::TalkingEvent(std::vector <sf::Texture> &otherTextures, sf::Vector2f const &position,
                                 std::vector <Utils::OpString> const &dialogKeys, int sides, EventTrigger eventTrigger,
                                 bool passable) :
        Event(otherTextures, eventTrigger, position, sides, passable), dialogKeys(dialogKeys){
        this->onLangChanged();
      }

      void TalkingEvent::onLangChanged(){
        dialogs.clear();
        for(auto &currentObj: this->dialogKeys){
          dialogs.push_back(currentObj.getString());
        }
      }

      LockedDoorEvent::LockedDoorEvent(std::vector <sf::Texture> &doorType, Item *needed, sf::Vector2f const &position,
                                       sf::Vector2i const &tpPos, string const &map, Side ppDir,
                                       EventTrigger eventTrigger, bool consumeItem, int sides, bool passable) :
        DoorEvent(doorType, position, tpPos, map, eventTrigger, ppDir, sides, passable),
        Event(this->otherTextures, eventTrigger, position, sides, passable),
        TalkingEvent(this->otherTextures, position, LockedDoorEvent::keysLock, sides, eventTrigger, passable),
        needed(needed), consumeItem(consumeItem){

      }

      CharacterEvent::CharacterEvent(std::string texturesKey, sf::Vector2f const &position, MoveStyle moveStyle,
                                     EventTrigger eventTrigger, std::vector <Side> predefinedPath, bool passable,
                                     int sides) :
        Event(Data::Ui::charaTextures[texturesKey], eventTrigger, position, sides, passable),
        moveStyle(moveStyle){
        sprite->setScale(2, 2);
        sprite->setOrigin(16, 16);
        sf::Vector2f posMap(((position.x + 8) * 32) + 16, (position.y + 8) * 32);
        sprite->setPosition(posMap);
        setPredefinedMove(predefinedPath);

      }

      TalkingCharaEvent::TalkingCharaEvent(std::string texturesKey, sf::Vector2f const &position,
                                           std::vector <Utils::OpString> const &dialogKeys, EventTrigger eventTrigger,
                                           MoveStyle moveStyle, std::vector <Side> predefinedPath, bool passable,
                                           int sides) :
        Event(Data::Ui::charaTextures[texturesKey], eventTrigger, position, sides, passable),
        CharacterEvent(texturesKey, position, moveStyle, eventTrigger, predefinedPath, passable, sides),
        TalkingEvent(Data::Ui::charaTextures[texturesKey], position, dialogKeys, sides, eventTrigger, passable){

      }

//Actions

      void TPEvent::action(Model::Player &player, View::Overworld& overworld){
        if(!justTP){
          overworld.tp(map, tpCoord);
          if(this->ppDir != -1){
            player.setppDir(this->ppDir);
          }
          justTP = true;
        }

      }

      void TPEvent::update(Model::Player &player, View::Overworld& overworld){

      }

      void DoorEvent::action(Model::Player &player, View::Overworld& overworld){
        animStarted = 0;
        if(doorType == 0){
          doorSound.setVolume(100);
          doorSound.play();
        }else if(doorType == 1){
          shopdoorSound.setVolume(300);
          shopdoorSound.play();
        }

      }

      void DoorEvent::update(Model::Player &player, View::Overworld& overworld){
        if(animStarted != -1){
          animStarted++;

          if(animStarted < 8 && (animStarted / 2) * 2 == animStarted){
            sprite->setTexture(otherTextures[animStarted / 2]);
          }else if(animStarted > 10){
            TPEvent::action(player, overworld);
            animStarted = -1;
            sprite->setTexture(otherTextures[0]);
          }

        }
      }

      void TalkingEvent::action(Model::Player &player, View::Overworld& overworld){
        overworld.startDialog(this->dialogs);
      }

      void TalkingEvent::update(Model::Player &player, View::Overworld& overworld){

      }

      void CharacterEvent::update(Model::Player &player, View::Overworld& overworld){
        frames++;
        if(anim == Side::NO_MOVE){
          int randomMove;
          switch(moveStyle){
            case MoveStyle::PREDEFINED:
              predefinedCounter++;
              if(predefinedCounter >= movements.size()){
                predefinedCounter = 0;
              }
              move(movements[predefinedCounter], player, overworld);
              break;

            case MoveStyle::NO_MOVE:
              break;

            case MoveStyle::RANDOM:
              randomMove = Utils::Misc::randUI(5) - 1;
              switch(randomMove){
                case -1:
                  move(Side::NO_MOVE, player, overworld);
                  break;
                case 0:
                  move(Side::TO_UP, player, overworld);
                  break;
                case 1:
                  move(Side::TO_DOWN, player, overworld);
                  break;
                case 2:
                  move(Side::TO_LEFT, player, overworld);
                  break;
                case 3:
                  move(Side::TO_RIGHT, player, overworld);
                  break;
                default:
                  Utils::Log::oplog("[WARNING] - Random number out of bounds CharacterEvent::update");
                  move(Side::NO_MOVE, player, overworld);
              }
              break;

            case MoveStyle::FOLLOWING:
              //TODO
              break;
          }
        }
        if(anim >= 0 && !anims){
          currentTexture = otherTextures.begin() + ((int) anim + 4);
          animsCounter++;
          anims = animsCounter > 8;
        }else if(anim >= 0 && anims){
          currentTexture = otherTextures.begin() + ((int) anim + 8);
          animsCounter++;
          if(animsCounter > 16){
            anims = false;
            animsCounter = 0;
          }
        }else if(anim < 0){
          currentTexture = otherTextures.begin() + (int) charaDir;
        }

        switch(anim){
          case Side::TO_UP:
            if(frames - startFrames >= 7){
              if(moving == Side::TO_UP){
                position -= sf::Vector2f(0, -4);//TODO : Find a solution about the coordinates problem (Map / Pixels)
              }
              anim = Side::NO_MOVE;
              moving = Side::NO_MOVE;
            }else{
              if(moving == Side::TO_UP){
                position -= sf::Vector2f(0, -4);
              }
            }
            break;


          case Side::TO_DOWN:
            if(frames - startFrames >= 7){
              if(moving == Side::TO_DOWN){
                position -= sf::Vector2f(0, 4);
              }
              anim = Side::NO_MOVE;
              moving = Side::NO_MOVE;
            }else{
              if(moving == Side::TO_DOWN){
                position -= sf::Vector2f(0, 4);
              }
            }
            break;


          case Side::TO_LEFT:
            if(frames - startFrames >= 7){
              if(moving == Side::TO_LEFT){
                position -= sf::Vector2f(-4, 0);
              }
              anim = Side::NO_MOVE;
              moving = Side::NO_MOVE;
            }else{
              if(moving == Side::TO_LEFT){
                position -= sf::Vector2f(-4, 0);
              }
            }
            break;


          case Side::TO_RIGHT:
            if(frames - startFrames >= 7){
              if(moving == Side::TO_RIGHT){
                position -= sf::Vector2f(4, 0);
              }
              anim = Side::NO_MOVE;
              moving = Side::NO_MOVE;
            }else{
              if(moving == Side::TO_RIGHT){
                position -= sf::Vector2f(4, 0);
              }
            }
            break;

          case Side::STAY:
            if(frames - startFrames >= 7){
              anim = Side::NO_MOVE;
            }
        };


      }

      void CharacterEvent::move(Side direction, Model::Player &player, View::Overworld& overworld){
        startFrames = frames;
        if(anim == Side::NO_MOVE && direction == Side::NO_MOVE){
          anim = Side::STAY;
          return;
        }
        if(anim == Side::NO_MOVE && direction != Side::NO_MOVE){
          anim = direction;
          charaDir = direction;
          switch(direction){
            case Side::TO_UP:
              if(position.y - 1 >= 0){
                if(overworld.current->getPassArr()[(int) position.y - 1][(int) position.x] == 0){
                  if(!(position.y - 1 == player.getPosY() && position.x == player.getPosX())){
                    for(Event *nextEvent : overworld.current->getEvent(
                      sf::Vector2i(position.x, position.y - 1))){
                      if(!nextEvent->isPassable()){
                        return;
                      }
                    }
                    moving = Side::TO_UP;
                    position.y--;
                  }else if(moveStyle == MoveStyle::PREDEFINED){
                    predefinedCounter--;
                  }

                }
              }
              break;
            case Side::TO_DOWN:
              if(position.y + 1 < overworld.current->getH()){
                if(overworld.current->getPassArr()[(int) position.y + 1][(int) position.x] == 0){
                  if(!(position.y + 1 == player.getPosY() && position.x == player.getPosX())){
                    for(Event *nextEvent : overworld.current->getEvent(
                      sf::Vector2i(position.x, position.y + 1))){
                      if(!nextEvent->isPassable()){
                        return;
                      }
                    }
                    moving = Side::TO_DOWN;
                    position.y++;
                  }else if(moveStyle == MoveStyle::PREDEFINED){
                    predefinedCounter--;
                  }

                }
              }
              break;

            case Side::TO_RIGHT:
              if(position.x + 1 < overworld.current->getW()){
                if(overworld.current->getPassArr()[(int) position.y][(int) position.x + 1] == 0){
                  if(!(position.x + 1 == player.getPosX() && position.y == player.getPosY())){
                    for(Event *nextEvent : overworld.current->getEvent(
                      sf::Vector2i(position.x + 1, position.y))){
                      if(!nextEvent->isPassable()){
                        return;
                      }
                    }
                    moving = Side::TO_RIGHT;
                    position.x++;
                  }else if(moveStyle == MoveStyle::PREDEFINED){
                    predefinedCounter--;
                  }

                }
              }
              break;

            case Side::TO_LEFT:
              if(position.x - 1 >= 0){
                if(overworld.current->getPassArr()[(int) position.y][(int) position.x - 1] == 0){
                  if(!(position.x - 1 == player.getPosX() && position.y == player.getPosY())){
                    for(Event *nextEvent : overworld.current->getEvent(
                      sf::Vector2i(position.x - 1, position.y))){
                      if(!nextEvent->isPassable()){
                        return;
                      }
                    }
                    moving = Side::TO_LEFT;
                    position.x--;
                  }else if(moveStyle == MoveStyle::PREDEFINED){
                    predefinedCounter--;
                  }

                }
              }
              break;
            default:
              break;
          }


        }
      }

      void TalkingCharaEvent::action(Model::Player &player, View::Overworld& overworld){
        overworld.movementLock = true;
        talking = true;

      }

      void TalkingCharaEvent::update(Model::Player &player, View::Overworld& overworld){
	
        if(overworld.movementLock && talking && !player.getPosition().isAnim()){
          switch(player.getPosition().getDir()){
            case Side::TO_UP:
              sprite->setTexture(otherTextures[(int) Side::TO_DOWN]);
              break;
            case Side::TO_DOWN:
              sprite->setTexture(otherTextures[(int) Side::TO_UP]);
              break;
            case Side::TO_LEFT:
              sprite->setTexture(otherTextures[(int) Side::TO_RIGHT]);
              break;
            case Side::TO_RIGHT:
              sprite->setTexture(otherTextures[(int) Side::TO_LEFT]);
              break;
          }
          overworld.movementLock = false;
          talking = false;
          overworld.startDialog(this->dialogs);
        }
        CharacterEvent::update(player, overworld);
      }

      void CharacterEvent::setPredefinedMove(std::vector <Side> moves){
        this->movements = moves;
      }

      void LockedDoorEvent::action(Model::Player &player, View::Overworld& overworld){

      }

      void LockedDoorEvent::update(Model::Player &player, View::Overworld& overworld){

      }

      std::vector <Utils::OpString> LockedDoorEvent::keysLock = std::vector<Utils::OpString>();


    }

    void initEnumsEvents(){
      Events::DoorType::NORMAL = Data::Ui::doorsTextures[0];
      Events::DoorType::SHOP = Data::Ui::doorsTextures[1];
    }

  }
}