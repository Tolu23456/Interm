import interm

def on_key(event_type):
    print(f"Python Plugin: Key pressed event received! (Type {event_type})")

interm.subscribe(interm.EVENT_KEY_PRESS, on_key)
print("Python Plugin: Core helper fully active with event subscription")
