#pragma once

void LoadTestScript(GameManager& g) {
    // Camera
    Entity* camera = g._entityManager->GetEntity(g._entityManager->AddEntity());
    {
        camera->_name = "camera";

        auto pTrans = camera->AddComponentOrDie<TransformComponent>().lock();

        float angle = 45.f * kDeg2Rad;
        Vec3 dir(0.f, sin(angle), cos(angle));
        float dist = 15.f;
        pTrans->SetPos(dist * dir);
        Mat3 rot = Mat3::FromAxisAngle(Vec3(1.f, 0.f, 0.f), -angle);
        pTrans->SetRot(rot);

        auto pCamera = camera->AddComponentOrDie<CameraComponent>().lock();
    }

    // Light
    Entity* light = g._entityManager->GetEntity(g._entityManager->AddEntity());
    {
        light->_name = "light";

        auto pTrans = light->AddComponentOrDie<TransformComponent>().lock();
        pTrans->SetPos(Vec3(0.f, 3.f, 0.f));

        auto pLight = light->AddComponentOrDie<LightComponent>().lock();
        pLight->_ambient.Set(0.2f, 0.2f, 0.2f);
        pLight->_diffuse.Set(1.f, 1.f, 1.f);
    }

    // // Cube1
    // Entity* cube1 = g._entityManager->AddEntity().lock().get();
    // {
    //     cube1->_name = "cube1";
    //     auto tComp = cube1->AddComponentOrDie<TransformComponent>().lock();
    //     auto modelComp = cube1->AddComponentOrDie<ModelComponent>().lock();
    //     modelComp->_modelId = std::string("wood_box");
    //     TransformComponent* t = tComp.get();
    //     auto beepComp = cube1->AddComponentOrDie<BeepOnHitComponent>().lock();
    //     beepComp->_synthChannel = 0;
    //     beepComp->_midiNotes = { 69, 73 };
    //     auto rbComp = cube1->AddComponentOrDie<RigidBodyComponent>().lock();
    //     rbComp->_localAabb = MakeCubeAabb(0.5f);
    //     auto velComp = cube1->AddComponentOrDie<VelocityComponent>().lock();
    //     velComp->_angularY = 2*kPi;

    //     cube1->ConnectComponentsOrDie(g);
    // }

    // // Cube2
    // Entity* cube2 = g._entityManager->AddEntity().lock().get();
    // cube2->_name = "cube2";
    // {
    //     auto tComp = cube2->AddComponentOrDie<TransformComponent>().lock();
    //     auto modelComp = cube2->AddComponentOrDie<ModelComponent>().lock();
    //     modelComp->_modelId = std::string("wood_box");
    //     TransformComponent* t = tComp.get();
    //     t->_transform.Translate(Vec3(-2.f,0.f,0.f));
    //     auto rbComp = cube2->AddComponentOrDie<RigidBodyComponent>().lock();
    //     rbComp->_localAabb = MakeCubeAabb(0.5f);
    //     rbComp->_static = false;
    //     auto controller = cube2->AddComponentOrDie<PlayerControllerComponent>().lock();

    //     cube2->ConnectComponentsOrDie(g);
    // }

    // // drone sequencer
    // Entity* droneSeq = g._entityManager->AddEntity().lock().get();
    // {
    //     droneSeq->_name = "drone_seq";
    //     auto seqComp = droneSeq->AddComponentOrDie<SequencerComponent>().lock();
    //     {
    //         audio::Event e;
    //         e.channel = 1;
    //         e.type = audio::EventType::NoteOn;
    //         e.midiNote = 45;
    //         e.timeInTicks = 0;
    //         seqComp->AddToSequence(e);

    //         e.midiNote = 49;
    //         seqComp->AddToSequence(e);

    //         e.midiNote = 52;
    //         seqComp->AddToSequence(e);
    //     }
    //     droneSeq->ConnectComponentsOrDie(g);
    // }

    // Init event queue with a synth sequence
    {
        // double const audioTime = Pa_GetStreamTime(audioContext._stream);
        // InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
        // InitEventQueueWithParamSequence(&audioContext._eventQueue, beatClock);
    }

    g._entityManager->ConnectComponents(g, /*dieOnConnectFailure=*/true);

}