#include "Calculator.h"
#include <string>
#include <cfloat>
#include <cmath>
#include <map>
#include <memory>

using namespace Pinetime::Applications::Screens;

namespace {
  template <typename X, uint8_t max_stack_len> struct Stack {
    // Basic stack data type without dynamic allocations
    X data[max_stack_len];
    uint8_t stack_len = 0;
    inline auto   size()   { return stack_len;                }
    inline bool   empty()  { return size() == 0;              }
    inline bool   full()   { return size() == max_stack_len;  }
    inline X&  top()    { return data[stack_len-1];        }
    inline X&  pop()    { return data[--stack_len];        }
    inline X&  push()   { return data[stack_len++];        }
    inline void push(X x) { X& datum{ push() }; datum = x; }
  };
  template <uint8_t max_stack_len> struct CalcStack : public Stack<double, max_stack_len> {
    typedef Stack<double, max_stack_len> Super;
    inline void pushValue(double value) {
      Super::push(value);
    }
    bool pushOperator(char op) {
      if (Super::size() < 2) return false;
      double* node0 = Super::data + Super::stack_len;
      // Call this only when stack_len >= 2
      // Evaluate subexpressions as soon as possible
      // Inf and NaN take care of error handling in case of non-finite results
      auto& val2 { node0[-2] };
      auto val1 = node0[-1];
      switch (op) {
      case '^':
        val2 = pow(val2, val1);
        break;
      case 'x':
        val2 *= val1;
        break;
      case '/':
        val2 /= val1;
        break;
      case '+':
        val2 += val1;
        break;
      case '-':
        val2 -= val1;
        break;
      default:
        return false;
      }
      Super::stack_len--;
      return true;
    }
  };

  template <typename Input, typename Float, typename Sign> inline bool parseFloat(Input& i, Float& f, Sign& s) {
    f = 0;
    int8_t dot_position = -1;
    while (!i.empty()) {
      auto& c = i.top();
      if ('0' <= c && c <= '9') {
        if (dot_position >= 0) dot_position++;
        f *= 10;
        f += c - '0';
      } else if ('.' == c) {
        if (dot_position >= 0) return false;
        dot_position = 0;
      } else break;
      i.pop();
    }
    while (dot_position-- > 0) f /= 10;
    if (s < 0) { f = -f; }
    return true;
  }

  uint8_t getPrecedence(char op) {
    switch (op) {
      case '^':
        return 4;
      case 'x':
      case '/':
        return 3;
      case '+':
      case '-':
        return 2;
    }
    return 0;
  }

  bool leftAssociative(char op) {
    switch (op) {
      case '^':
        return false;
      case 'x':
      case '/':
      case '+':
      case '-':
        return true;
    }
    return false;
  }
};

static void eventHandler(lv_obj_t* obj, lv_event_t event) {
  auto calc = static_cast<Calculator*>(obj->user_data);
  calc->OnButtonEvent(obj, event);
}

Calculator::~Calculator() {
  lv_obj_clean(lv_scr_act());
}

static const char* buttonMap1[] = {
  "7", "8", "9", "/", "\n",
  "4", "5", "6", "x", "\n",
  "1", "2", "3", "-", "\n",
  ".", "0", "=", "+", "",
};

static const char* buttonMap2[] = {
  "7", "8", "9", "(", "\n",
  "4", "5", "6", ")", "\n",
  "1", "2", "3", "^", "\n",
  ".", "0", "=", "+", "",
};

Calculator::Calculator(DisplayApp* app, Controllers::MotorController& motorController) : Screen(app), motorController {motorController} {
  result = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(result, LV_LABEL_LONG_BREAK);
  lv_label_set_text_static(result, "0");
  lv_obj_set_size(result, 180, 60);
  lv_obj_set_pos(result, 0, 0);

  returnButton = lv_btn_create(lv_scr_act(), nullptr);
  lv_obj_set_size(returnButton, 52, 52);
  lv_obj_set_pos(returnButton, 186, 0);
  lv_obj_t* returnLabel;
  returnLabel = lv_label_create(returnButton, nullptr);
  lv_label_set_text_static(returnLabel, "<=");
  lv_obj_align(returnLabel, nullptr, LV_ALIGN_CENTER, 0, 0);
  returnButton->user_data = this;
  lv_obj_set_event_cb(returnButton, eventHandler);

  buttonMatrix = lv_btnmatrix_create(lv_scr_act(), nullptr);
  lv_btnmatrix_set_map(buttonMatrix, buttonMap1);
  lv_obj_set_size(buttonMatrix, 240, 180);
  lv_obj_set_pos(buttonMatrix, 0, 60);
  lv_obj_set_style_local_pad_all(buttonMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 0);
  buttonMatrix->user_data = this;
  lv_obj_set_event_cb(buttonMatrix, eventHandler);
}

bool Calculator::Eval() {
  // The given sizes should be enough for parsing 30 characters
  Stack<char, 32> input;
  CalcStack<16> output;
  Stack<char, 32> operators;
  for (int8_t i = position - 1; i >= 0; i--) {
    input.push(text[i]);
  }
  bool expectingNumber = true;
  int8_t sign = +1;
  double resultFloat;
  uint32_t lower, upper;
  while (!input.empty()) {
    if (input.top() == '.') {
      input.push('0');
    }

    if (isdigit(input.top())) {
      if (!parseFloat(input, output.push(), sign)) return false;
      sign = +1;
      expectingNumber = false;
      continue;
    }

    if (expectingNumber) {
      switch (input.top()) {
        case '-':
          sign *= -1;
          [[fallthrough]];
        case '+':
          input.pop();
          continue;
      }
    }

    char next = input.top();
    input.pop();

    switch (next) {
      case '+':
      case '-':
      case '/':
      case 'x':
      case '^':
        // while ((there is an operator at the top of the operator stack)
        while (!operators.empty()
               // and (the operator at the top of the operator stack is not a left parenthesis))
               && operators.top() != '('
               // and ((the operator at the top of the operator stack has greater precedence)
               && (getPrecedence(operators.top()) > getPrecedence(next)
                   // or (the operator at the top of the operator stack has equal precedence and the token is left associative))
                   || (getPrecedence(operators.top()) == getPrecedence(next) && leftAssociative(next)))) {
          // need two elements on the output stack to add a binary operator
          if (!output.pushOperator(operators.top())) return false;
          operators.pop();
        }
        operators.push(next);
        expectingNumber = true;
        break;
      case '(':
        if (expectingNumber) {
          if (sign < 0) {
            // Handle correctly expressions like: '-(5+11)' or '2*-(5+11)'
            sign = 1;
            output.pushValue(0);
            operators.push('-');
          }
        } else {
          // We expect there to be a binary operator here but found a left parenthesis.
          // This occurs in terms like this: a+b(c).
          // This should be interpreted as a+b*(c)
          operators.push('x');
        }
        operators.push(next);
        expectingNumber = true;
        break;
      case ')':
        while (operators.top() != '(') {
          // need two elements on the output stack to add a binary operator
          if (!output.pushOperator(operators.top())) return false;
          operators.pop();
          if (operators.empty()) return false;
        }
        // discard the left parentheses
        operators.pop();
    }
  }
  while (!operators.empty()) {
    if (!output.pushOperator(operators.top())) return false;
    operators.pop();
  }
  // perform the calculation
  // assert(output.size() == 1);
  resultFloat = output.data[0]; // this is the same as output.top() when output.size() == 1, but may reduce binary size
  // make sure that the absolute value of the integral part of result fits in a 32 bit unsigned integer
  sign = (resultFloat < 0);
  if (sign) {
    resultFloat = -resultFloat;
  }
  if (!(resultFloat <= UINT32_MAX)) {
    return false; // if too large or NaN
  }
  if (sign) {
    text[0] = '-';
    position = 1;
  } else {
    position = 0;
  }
  // workaround: provided sprintf doesn't support floats
  upper = resultFloat;
  lower = round((resultFloat - upper) * 1000000);
  if (lower >= 1000000) {
    lower = 0;
    upper++;
  }
  position += sprintf(text + position, "%lu", upper);
  if (lower != 0) {
    // see if decimal places have to be printed
    position += sprintf(text + position, ".%06lu", lower);
    // remove extra zeros
    while (text[position - 1] == '0') {
      position--;
    }
  }
  return true;
}

void Calculator::Reset() {
  position = 0;
  lv_label_set_text_static(result, "0");
}

void Calculator::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  switch (event) {
  case LV_EVENT_CLICKED:
    if (obj == buttonMatrix) {
      const char* buttonstr = lv_btnmatrix_get_active_btn_text(obj);
      if (*buttonstr == '=') {
        if (!Eval()) break;
      } else {
        if (position >= 30) break;
        text[position] = *buttonstr;
        position++;
      }
    } else if (obj == returnButton) {
      if (position > 1) {
        position--;
      } else {
        Reset();
        return;
      }
    }
    text[position] = '\0';
    lv_label_set_text_static(result, text);
    [[fallthrough]];
  default:
    return;
  }
  motorController.RunForDuration(10);
}

bool Calculator::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  switch (event) {
    case Pinetime::Applications::TouchEvents::LongTap:
      Reset();
      break;
    case Pinetime::Applications::TouchEvents::SwipeLeft:
      lv_btnmatrix_set_map(buttonMatrix, buttonMap2);
      break;
    case Pinetime::Applications::TouchEvents::SwipeRight:
      lv_btnmatrix_set_map(buttonMatrix, buttonMap1);
      break;
    default:
      return false;
  }
  return true;
}
