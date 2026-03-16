with open('android/app/src/main/cpp/main_android.cpp', 'r') as f:
    s = f.read()

old = '''            Vector3 camFwd=GetFwd();
            camFwd.x=-camFwd.x; camFwd.z=-camFwd.z; // negate to match LookAt view space
            camFwd.y=0.f;
            float fwdLen=sqrtf(camFwd.x*camFwd.x+camFwd.z*camFwd.z);
            if(fwdLen<0.001f){camFwd={0.f,0.f,-1.f};}
            else{camFwd.x/=fwdLen;camFwd.z/=fwdLen;}
            Vector3 camRight=camFwd.Cross({0.f,1.f,0.f}).Normalized();
            // joyX = right, joyY = down on screen
            // joystick up = -joyY = forward, joystick right = joyX = right
            float moveForward=-joyY;
            float moveRight=joyX;
            float rawInputLen=sqrtf(moveForward*moveForward+moveRight*moveRight);
            if(rawInputLen>1.f){moveForward/=rawInputLen;moveRight/=rawInputLen;}
            // camera forward and right are correct, just apply directly
            // forward = camFwd * moveForward
            // strafe  = camRight * moveRight
            pos.x+=(camFwd.x*moveForward+camRight.x*moveRight)*spd*3.f;
            pos.z+=(camFwd.z*moveForward+camRight.z*moveRight)*spd*3.f;'''

new = '''            // Get camera forward flattened to ground plane
            Vector3 camFwd=GetFwd();
            camFwd.y=0.f;
            float fwdLen=sqrtf(camFwd.x*camFwd.x+camFwd.z*camFwd.z);
            if(fwdLen<0.001f){camFwd={0.f,0.f,-1.f};}
            else{camFwd.x/=fwdLen;camFwd.z/=fwdLen;}
            // Right = forward cross up
            Vector3 camRight=camFwd.Cross({0.f,1.f,0.f}).Normalized();
            // joyY: negative = finger up = move forward
            // joyX: positive = finger right = strafe right
            float moveForward=-joyY;
            float moveRight=joyX;
            float rawInputLen=sqrtf(moveForward*moveForward+moveRight*moveRight);
            if(rawInputLen>1.f){moveForward/=rawInputLen;moveRight/=rawInputLen;}
            pos.x+=(camFwd.x*moveForward+camRight.x*moveRight)*spd*3.f;
            pos.z+=(camFwd.z*moveForward+camRight.z*moveRight)*spd*3.f;'''

if old in s:
    s = s.replace(old, new)
    print("Fixed! Removed wrong negate")
else:
    print("ERROR: not found")

with open('android/app/src/main/cpp/main_android.cpp', 'w') as f:
    f.write(s)
print("Done")
