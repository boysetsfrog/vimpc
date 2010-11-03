#include "commands.hpp"

#include <limits>
#include <sstream>
#include <algorithm>

#include "console.hpp"
#include "dbc.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

using namespace Ui;

int  const PromptSize      = 1;
char const CommandPrompt[] = ":";

// COMMANDS
Commands::Commands(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   Player             (screen, client),
   settings_          (settings),
   window_            (NULL),
   cursor_            (command_),
   command_           (""),
   initHistorySearch_ (true),
   initTabCompletion_ (true)
{
   // \todo add :quit! to quit and force song stop ?
   // \todo find a away to add aliases to tab completion
   // \todo allow aliases to be multiple commands, ie alias quit! stop; quit
   // or something similar
   commandTable_["!mpc"]      = &Commands::Mpc;
   commandTable_["alias"]     = &Commands::Alias;
   commandTable_["clear"]     = &Commands::ClearScreen;
   commandTable_["console"]   = &Commands::Console;
   commandTable_["connect"]   = &Commands::Connect;
   commandTable_["echo"]      = &Commands::Echo;
   commandTable_["help"]      = &Commands::Help;
   commandTable_["library"]   = &Commands::Library;
   commandTable_["next"]      = &Commands::Next;
   commandTable_["pause"]     = &Commands::Pause;
   commandTable_["play"]      = &Commands::Play;
   commandTable_["playlist"]  = &Commands::Playlist;
   commandTable_["previous"]  = &Commands::Previous;
   commandTable_["quit"]      = &Commands::Quit;
   commandTable_["random"]    = &Commands::Random;
   commandTable_["set"]       = &Commands::Set;
   commandTable_["stop"]      = &Commands::Stop;

   window_ = screen_.CreateModeWindow();
}

Commands::~Commands()
{
   delete window_;
   window_ = NULL;
}


void Commands::InitialiseMode()
{
   initHistorySearch_  = true;
   initTabCompletion_  = true;

   window_->ShowCursor();
   window_->SetCursorPosition(cursor_.Position());
   window_->SetLine(CommandPrompt);

   ENSURE(command_.empty() == true);
}

void Commands::FinaliseMode()
{
   AddCommandToHistory(command_);

   command_.clear();
   cursor_.ResetCursorPosition();
   window_->HideCursor();
   window_->SetLine("");

   ENSURE(command_.empty() == true);
}

bool Commands::Handle(int const input)
{
   bool result = true;

   if (HasCommandToExecute(input) == true)
   {
      result = ExecuteCommand(command_);

      screen_.Update();
   }
   else
   {
      ResetHistory(input);
      ResetTabCompletion(input);
      GenerateCommandFromInput(input);

      int64_t const newCursorPosition = cursor_.UpdatePosition(input);

      window_->SetCursorPosition(newCursorPosition);
      window_->SetLine("%s%s", CommandPrompt, command_.c_str());
   }

   return result;
}


bool Commands::ExecuteCommand(std::string const & input)
{
   bool result = true;

   std::string command, arguments;

   SplitCommand(input, command, arguments);

   if (aliasTable_.find(command) != aliasTable_.end())
   {
      std::string resolvedAlias(aliasTable_[command]);
      resolvedAlias.append(" ");
      resolvedAlias.append(arguments);

      result = ExecuteCommand(resolvedAlias);
   }
   else
   {
      result = ExecuteCommand(command, arguments);
   }

   return result;
}

bool Commands::HasCommandToExecute(int input) const
{
   return (input == '\n');
}

bool Commands::InputIsValidCharacter(int input)
{
   return (input < std::numeric_limits<char>::max()) 
       && (input != 27) 
       && (input != '\n');
}

void Commands::GenerateCommandFromInput(int input)
{
   // \todo this could use a refactor
   int64_t const cursorPosition = (cursor_.Position() - PromptSize);

   if ((input == KEY_UP) || (input == KEY_DOWN))
   {
      Direction const direction = (input == KEY_UP) ? Up : Down;
      command_ = History(direction, command_);
   }
   else if (input == '\t')
   {
      command_ = TabComplete(command_);
   }
   else if (((input == KEY_BACKSPACE) || (input == KEY_DC)) && ((command_.empty() == false)))
   {
      const int cursorMovement = (input == KEY_BACKSPACE) ? 1 : 0;

      if ((cursorPosition - cursorMovement) >= 0)
      {
         command_.erase((cursorPosition - cursorMovement), 1);
      }
   }
   else if (InputIsValidCharacter(input) == true)
   {
      command_.insert((std::string::size_type) (cursorPosition), 1, (char) input);
   }
}
 
bool Commands::ExecuteCommand(std::string const & command, std::string const & arguments)
{
   std::string commandToExecute  = command;
   bool        result            = true;
   bool        matchingCommand   = (commandTable_.find(commandToExecute) != commandTable_.end());

   // If we can't find the exact command, look for a unique command that starts
   // with the input command
   if (matchingCommand == false)
   {
      uint32_t validCommandCount = 0;

      for (CommandTable::const_iterator it = commandTable_.begin(); it != commandTable_.end(); ++it)
      {
         if (command.compare(it->first.substr(0, command.length())) == 0)
         {
            ++validCommandCount;
            commandToExecute = it->first;
         }
      }

      matchingCommand = (validCommandCount == 1);
   }

   // If we have found a command execute it, with \p arguments
   if (matchingCommand == true)
   {
      CommandTable::const_iterator it = commandTable_.find(commandToExecute);
      ptrToMember commandFunc = it->second;
      result = (*this.*commandFunc)(arguments);
   }

   return result;
}

void Commands::SplitCommand(std::string const & input, std::string & command, std::string & arguments)
{
   std::stringstream commandStream(input);
   std::getline(commandStream, command,   ' '); 
   std::getline(commandStream, arguments, '\n');
}

bool Commands::Set(std::string const & arguments)
{
   settings_.Set(arguments);

   // \todo work out wtf i am doing with these returns, seriously
   return true;
}

bool Commands::Mpc(std::string const & arguments)
{
   static uint32_t const bufferSize = 512;
   char   buf[bufferSize];

   std::string command("mpc ");
   command.append(arguments);

   screen_.ConsoleWindow().OutputLine("> %s", command.c_str());

   FILE * mpcOutput = popen(command.c_str(), "r");

   if (mpcOutput != NULL)
   {
      while (fgets(buf, bufferSize - 1, mpcOutput) != NULL)
      {
         screen_.ConsoleWindow().OutputLine("%s", buf);
      }

      pclose(mpcOutput);
   }

   // \todo
   return true;
}

bool Commands::Alias(std::string const & input)
{
   std::string command, arguments;

   SplitCommand(input, command, arguments);

   aliasTable_[command] = arguments;

   //\todo 
   return true;
}


void Commands::ResetHistory(int input)
{
   if ((input != KEY_UP) && (input != KEY_DOWN))
   {
      initHistorySearch_ = true;
   }
}

void Commands::ResetTabCompletion(int input)
{
   if (input != '\t')
   {
      initTabCompletion_ = true;
   }
}


void Commands::AddCommandToHistory(std::string const & command)
{
   if (command.empty() == false)
   {
      //Remove this command if it is already in the history
      commandHistory_.erase(std::remove(commandHistory_.begin(), 
                                        commandHistory_.end(), 
                                        command),
                            commandHistory_.end());

      //Then add it to the back (as the most recent command)
      commandHistory_.push_back(command);
   }
}

void Commands::InitialiseHistorySearch(std::string const & command)
{
   searchCommandHistory_.clear();
   searchCommandHistory_.reserve(commandHistory_.size());

   std::remove_copy_if(commandHistory_.begin(), 
                       commandHistory_.end(),
                       std::back_inserter(searchCommandHistory_),
                       HistoryNotCompletionMatch(command));
}

std::string Commands::History(Direction direction, std::string const & command)
{
   static CommandHistory::const_reverse_iterator historyIterator;
   static std::string historyLastResult(command);
   static std::string historyStart     (command);

   std::string result;

   if (initHistorySearch_ == true)
   {
      initHistorySearch_   = false;
      historyStart         = command;

      InitialiseHistorySearch(historyStart);

      historyIterator = searchCommandHistory_.rbegin();
      --historyIterator;
   }

   if (historyIterator == searchCommandHistory_.rend())
   {
      --historyIterator;
   }

   if (direction == Up)
   {
      if (historyIterator < searchCommandHistory_.rend())
      {
         ++historyIterator;
      }

      result = ((historyIterator == searchCommandHistory_.rend()) ? historyLastResult : *historyIterator);
   }
   else
   {
      if (historyIterator >= searchCommandHistory_.rbegin())
      {
         --historyIterator;
      }

      result = ((historyIterator < searchCommandHistory_.rbegin()) ? historyStart : *historyIterator);
   }

   historyLastResult = result;

   return result;
}


std::string Commands::TabComplete(std::string const & command)
{
   static std::string tabStart(command);
   static CommandTable::iterator tabIterator(commandTable_.begin());

   std::string result;

   if (initTabCompletion_ == true)
   {
      initTabCompletion_ = false;
      tabIterator        = commandTable_.begin();
      tabStart           = command;
   }

   tabIterator = find_if(tabIterator, commandTable_.end(), TabCompletionMatch(tabStart));

   if (tabIterator != commandTable_.end())
   {
      result = tabIterator->first;
      ++tabIterator;
   }
   else
   {
      initTabCompletion_ = true;
      result             = tabStart;
   }

   return result;
}


// Helper Classes
// CURSOR
CommandCursor::CommandCursor(std::string & command) :
   input_   (0),
   position_(PromptSize),
   command_ (command)
{
}

CommandCursor::~CommandCursor()
{
}

uint16_t CommandCursor::Position()
{
   return position_;
}

uint16_t CommandCursor::UpdatePosition(int input)
{
   uint16_t const minCursorPosition = PromptSize;
   uint16_t const maxCursorPosition = (command_.size() + PromptSize);

   input_ = input;

   CursorState newCursorState = CursorMovementState();

   //Determine where the cursor should move to
   switch (newCursorState)
   {
      case CursorLeft:
         --position_;
         break;

      case CursorRight:
         ++position_;
         break;

      case CursorStart:
         position_ = minCursorPosition;
         break;

      case CursorEnd:
         position_ = maxCursorPosition;
         break;

      default:
         break;
   }

   position_ = LimitCursorPosition(position_);

   return position_;
}

void CommandCursor::ResetCursorPosition()
{
   position_ = PromptSize;
}

bool CommandCursor::WantCursorLeft()
{
   return ((input_ == KEY_LEFT) ||
           (input_ == KEY_BACKSPACE));
}

bool CommandCursor::WantCursorEnd()
{
   return ((input_ == KEY_UP)   || 
           (input_ == KEY_DOWN) || 
           (input_ == '\t'));
}

bool CommandCursor::WantCursorRight()
{
   return ((input_ == KEY_RIGHT) || 
           (Commands::InputIsValidCharacter(input_) == true));
}

bool CommandCursor::WantCursorStart()
{
   return ((input_ == KEY_HOME));
}

CommandCursor::CursorState CommandCursor::CursorMovementState()
{
   CursorState newCursorState = CursorNoMovement;

   if (WantCursorStart() == true)
   {
      newCursorState = CursorStart;
   }
   else if (WantCursorEnd() == true)
   {
      newCursorState = CursorEnd;
   }
   else if (WantCursorLeft() == true)
   {
      newCursorState = CursorLeft;
   }
   else if (WantCursorRight() == true)
   {
      newCursorState = CursorRight;
   }

   return newCursorState;
}

uint16_t CommandCursor::LimitCursorPosition(uint16_t position) const
{
   uint16_t const minCursorPosition = PromptSize;
   uint16_t const maxCursorPosition = (command_.size() + PromptSize);

   //Ensure that the cursor is in a valid range
   if (position_ < minCursorPosition)
   {
      position = minCursorPosition;
   }
   else if (position_ > maxCursorPosition)
   {
      position = maxCursorPosition;
   }

   return position;
}
