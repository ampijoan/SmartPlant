#ifndef _BUTTON_H_
#define _BUTTON_H_

class Button {
  int _buttonPin;
  int _prevButtonState;

  public:
    Button(int buttonPin) {
      _buttonPin = buttonPin;
      pinMode(_buttonPin,INPUT);
    }

    bool isPressed() {
      return digitalRead(_buttonPin);
    }

    bool isClicked() {
      bool _buttonState, _clicked;

      _buttonState = digitalRead(_buttonPin);
      if(_buttonState != _prevButtonState) {
        _clicked = _buttonState;
      }
      else {
        _clicked = false;
      }
      _prevButtonState=_buttonState;
      return _clicked;
    }
};

#endif // _BUTTON_H_