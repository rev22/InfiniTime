#include "Calculator.h"
#include <string>
#include <cfloat>
#include <cmath>
#include <map>
#include <memory>
#include <string>

using namespace Pinetime::Applications::Screens;

// Anonymous Namespace for all the structs
namespace {
  struct Node {
    char op;
    double val;
  };
  template <typename X, uint8_t max_stack_len> struct StaticallyAllocatedStack {
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
  template <uint8_t max_stack_len> struct CalcStack : public StaticallyAllocatedStack<Node, max_stack_len> {
    typedef StaticallyAllocatedStack<Node, max_stack_len> Super;
    inline Node& push() { return Super::push(); }
    inline void pushValue(double value) {
      Super::data[Super::stack_len].op = 0;
      Super::data[Super::stack_len].val = value;
      Super::stack_len++;
    }
    void pushOperator(char op) {
      Node* node0 = Super::data + Super::stack_len;
      node0->op = op;
      if (Super::stack_len > 1) {
        Node* node1 = node0 - 1;
        if (node1->op == 0) {
          Node* node2 = node1 - 1;
          auto val2 = node2->val;
          auto val1 = node1->val;
          if (node2->op == 0) {
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
              goto done;
            }
          }
          node2->val = val2;
          Super::stack_len--;
          return;
        } 
      }
     done:
      Super::stack_len++;
    }
  };

  template <typename I, typename F, typename S> bool parseFloat(I& i, F& f, S& s) {
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

void Calculator::Eval() {
  StaticallyAllocatedStack<char, 32> input;
  for (int8_t i = position - 1; i >= 0; i--) {
    input.push(text[i]);
  }
  CalcStack<16> output;
  StaticallyAllocatedStack<char, 32> operators;
  bool expectingNumber = true;
  int8_t sign = +1;
  double resultFloat;
  uint32_t lower, upper;
  while (!input.empty()) {
    if (input.top() == '.') {
      input.push('0');
    }

    if (isdigit(input.top())) {
      if (!parseFloat(input, output.push().val, sign)) {
        goto eval_error;
      }
      output.top().op = 0;
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
          break;
        default:
          goto not_sign;
      }
      continue;
     not_sign: ;
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
          if (output.size() < 2) {
            goto eval_error;
          }
          output.pushOperator(operators.top());
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
          if (output.size() < 2) {
            goto eval_error;
          }
          output.pushOperator(operators.top());
          operators.pop();
          if (operators.empty()) {
            goto eval_error;
          }
        }
        // discard the left parentheses
        operators.pop();
    }
  }
  while (!operators.empty()) {
    char op = operators.pop();
    if ((op == ')' || op == '(')
        ||
        // need two elements on the output stack to add a binary operator
        (output.size() < 2))
    {
      goto eval_error;
    }
    output.pushOperator(op);
  }
  // perform the calculation
  resultFloat = output.top().val;
#if 0
  // This only seems to work in the simulator
  if (!std::isfinite(resultFloat)) {
    goto eval_error;
  }
  position = 0;
  for (char s : std::to_string(resultFloat)) {
    text[position++] = s;
  }
  text[position] = 0;
#else
  // position = snprintf(text, 20, "%.9g", resultFloat);
  // make sure that the absolute value of the integral part of result fits in a 32 bit unsigned integer
  sign = (resultFloat < 0);
  if (sign) {
    resultFloat = -resultFloat;
  }
  if (!(resultFloat <= UINT32_MAX)) {
    goto eval_error; // if too large or NaN
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
#endif
  return;
 eval_error:
  motorController.RunForDuration(10);
}

void Calculator::Reset() {
  position = 0;
  lv_label_set_text_static(result, "0");
}

void Calculator::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    if (obj == buttonMatrix) {
      const char* buttonstr = lv_btnmatrix_get_active_btn_text(obj);
      if (*buttonstr == '=') {
        Eval();
      } else {
        if (position >= 30) {
          motorController.RunForDuration(10);
          return;
        }
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
  }
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
